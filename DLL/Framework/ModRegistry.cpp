#include <iomanip>

#include "../RSColor.h"
#include "ModRegistry.hpp"

#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "ModContext.hpp"
#include "../Log.hpp"

namespace Framework {
	PendingRegistration* g_modPendingHead = nullptr;

	enum class ModState {
		Registered,
		Inactive,
		Active,
		Faulted,
	};

	struct ModRegistry::Impl {
		using Hook = void (IMod::*)(ModContext&);

		struct Record {
			std::unique_ptr<IMod> mod;
			ModState state = ModState::Registered;
			bool inSong = false;
		};

		bool Invoke(Record& record, Hook hook, const char* where) {
			try {
				(record.mod.get()->*hook)(ctx);
				return true;
			}
			catch (const std::exception& ex) {
				LOG_ERROR("[Framework] " << record.mod->Id() << "::" << where << " threw: " << ex.what() << std::endl);
			}
			catch (...) {
				LOG_ERROR("[Framework] " << record.mod->Id() << "::" << where << " threw unknown exception" << std::endl);
			}

			return false;
		}

		bool EnabledSafe(Record& record) {
			try {
				return record.mod->IsEnabled(ctx);
			}
			catch (...) {
				LOG_ERROR("[Framework] " << record.mod->Id() << "::IsEnabled threw; treating as disabled" << std::endl);
				return false;
			}
		}

		void Fault(Record& record, const char* reason) {
			LOG_ERROR("[Framework] " << record.mod->Id() << " faulted (" << reason << "); it will no longer run" << std::endl);
			record.state = ModState::Faulted;
		}

		// Best-effort revert of live game state before a mod leaves Active.
		void Revert(Record& record) {
			if (record.inSong) {
				Invoke(record, &IMod::OnSongExit, "OnSongExit");
				record.inSong = false;
			}
			Invoke(record, &IMod::OnDisabled, "OnDisabled");
		}

		void Deactivate(Record& record) {
			Revert(record);
			record.state = ModState::Inactive;

			LOG_INFO("[Framework] " << record.mod->Id() << " disabled" << std::endl);
		}

		void Activate(Record& record) {
			if (!Invoke(record, &IMod::OnEnabled, "OnEnabled")) {
				Fault(record, "threw in OnEnabled");
				return;
			}

			record.state = ModState::Active;
			LOG_INFO("[Framework] " << record.mod->Id() << " activated" << std::endl);
		}

		// Runs an active-only hook; on throw, best-effort revert then fault immediately.
		bool InvokeActive(Record& record, Hook hook, const char* where) {
			if (Invoke(record, hook, where)) {
				return true;
			}

			Revert(record);
			Fault(record, "threw in a tick hook");

			return false;
		}

		void ReconcileSongAndTick(Record& record, GamePhase phase) {
			const bool songPhase = phase == GamePhase::Song;

			if (songPhase && !record.inSong) {
				if (!InvokeActive(record, &IMod::OnSongEnter, "OnSongEnter")) return;
				record.inSong = true;
			}
			else if (!songPhase && record.inSong) {
				Invoke(record, &IMod::OnSongExit, "OnSongExit");
				record.inSong = false;
			}

			if (!InvokeActive(record, &IMod::OnTick, "OnTick")) return;
			if (phase == GamePhase::Menu && !InvokeActive(record, &IMod::OnMenuTick, "OnMenuTick")) return;
			if (phase == GamePhase::Song) InvokeActive(record, &IMod::OnSongTick, "OnSongTick");
		}

		void ProcessRecord(Record& record, GamePhase phase) {
			switch (record.state) {
				case ModState::Registered:
				case ModState::Faulted:
					return;

				case ModState::Inactive:
					if (!EnabledSafe(record)) return;
					Activate(record);
					break;

				case ModState::Active:
					if (!EnabledSafe(record)) {
						Deactivate(record);
						return;
					}
					break;
			}

			if (record.state == ModState::Active)
				ReconcileSongAndTick(record, phase);
		}

		void DrainSettings() {
			std::vector<std::function<void()>> batch;
			{
				std::lock_guard<std::mutex> lock(settingsMutex);
				batch.swap(pendingSettings);
			}
			if (batch.empty())
				return;

			for (auto& apply : batch) {
				try {
					apply();
				}
				catch (...) {
					LOG_ERROR("[Framework] a queued settings update threw while applying" << std::endl);
				}
			}
			for (auto& record : records) {
				if (record.state != ModState::Registered && record.state != ModState::Faulted)
					Invoke(record, &IMod::OnSettingsChanged, "OnSettingsChanged");
			}
		}

		std::vector<Record> records;
		ModContext ctx;
		std::mutex settingsMutex;
		std::vector<std::function<void()>> pendingSettings;
	};

	ModRegistry::ModRegistry() : impl(std::make_unique<Impl>()) {}
	ModRegistry::~ModRegistry() = default;

	void ModRegistry::InstantiatePending() {
		for (PendingRegistration* pending = g_modPendingHead; pending; pending = pending->next) {
			if (pending->factory)
				Register(pending->factory());
		}
	}

	void ModRegistry::Register(std::unique_ptr<IMod> mod) {
		const std::string_view id = mod->Id();
		
		for (const auto& record : impl->records) {
			if (record.mod->Id() == id) {
				LOG_ERROR("[Framework] Duplicate mod Id '" << id << "' — registration rejected" << std::endl);
				return;
			}
		}

		LOG_INFO("[Framework] Registered mod: " << id << std::endl);
		impl->records.push_back(Impl::Record{ std::move(mod) });
	}

	void ModRegistry::DispatchInitialize() {
		for (auto& record : impl->records) {
			if (record.state != ModState::Registered) continue;
			if (impl->Invoke(record, &IMod::OnInitialize, "OnInitialize")) {
				record.state = ModState::Inactive;
			}
			else {
				impl->Fault(record, "threw in OnInitialize");
			}
		}
	}

	void ModRegistry::EnqueueSettingsUpdate(std::function<void()> apply) {
		std::lock_guard<std::mutex> lock(impl->settingsMutex);
		impl->pendingSettings.push_back(std::move(apply));
	}

	void ModRegistry::Tick(GamePhase phase, GameLoopState& loop) {
		impl->ctx.phase = phase;
		impl->ctx.loop = &loop;

		impl->DrainSettings();

		for (auto& record : impl->records)
			impl->ProcessRecord(record, phase);
	}

	void ModRegistry::Shutdown() {
		for (auto& record : impl->records) {
			if (record.state == ModState::Registered)
				continue;

			if (record.state == ModState::Active) impl->Revert(record);

			impl->Invoke(record, &IMod::OnShutdown, "OnShutdown");
		}

		impl->records.clear();
	}

	ModRegistry& Registry() {
		static ModRegistry instance;
		return instance;
	}
}
