#include <iomanip>

#include "../RSColor.h"
#include "ModRegistry.hpp"

#include <algorithm>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "ConflictResolver.hpp"
#include "ModContext.hpp"
#include "../Log.hpp"

namespace Framework {
	PendingRegistration* g_modPendingHead = nullptr;

	enum class ModState {
		Registered,
		Inactive,   // Initialized but not effectively active: never-activated, user-disabled, or conflict-suppressed.
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

		void Deactivate(Record& record, bool suppressed) {
			Revert(record);
			record.state = ModState::Inactive;
			LOG_INFO("[Framework] " << record.mod->Id()
				<< (suppressed ? " suppressed by a higher-priority conflicting mod" : " disabled") << std::endl);
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

		bool IsActivatable(ModState state) const {
			return state == ModState::Inactive || state == ModState::Active;
		}

		void BuildResolverIfNeeded() {
			if (!resolverDirty)
				return;

			modResources.assign(records.size(), {});
			for (size_t i = 0; i < records.size(); ++i) {
				for (std::string_view resource : records[i].mod->ClaimsExclusive()) {
					auto& resources = modResources[i];
					const std::string owned(resource);
					if (std::find(resources.begin(), resources.end(), owned) == resources.end())
						resources.push_back(owned);
				}
			}
			resolverDirty = false;
		}

		void ComputeRawEnabled(std::vector<char>& rawEnabled) {
			rawEnabled.assign(records.size(), 0);
			for (size_t i = 0; i < records.size(); ++i) {
				if (IsActivatable(records[i].state))
					rawEnabled[i] = EnabledSafe(records[i]);
			}
		}

		void ResolveDesired(const std::vector<char>& rawEnabled, std::vector<char>& desired) {
			std::vector<Resolver::Candidate> candidates(records.size());
			for (size_t i = 0; i < records.size(); ++i) {
				candidates[i].id = records[i].mod->Id();
				candidates[i].priority = records[i].mod->Priority();
				candidates[i].enabled = rawEnabled[i] && IsActivatable(records[i].state);
				candidates[i].resources = &modResources[i];
			}

			Resolver::Resolve(candidates, desired);
		}

		std::vector<Record> records;
		ModContext ctx;
		std::mutex settingsMutex;
		std::vector<std::function<void()>> pendingSettings;
		bool resolverDirty = false;
		std::vector<std::vector<std::string>> modResources;
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
		impl->resolverDirty = true;
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
		impl->BuildResolverIfNeeded();

		std::vector<char> rawEnabled;
		std::vector<char> desired;
		impl->ComputeRawEnabled(rawEnabled);
		impl->ResolveDesired(rawEnabled, desired);

		// Pass 1: deactivate losers before activating winners, so a loser's OnDisabled reverts
		// its game state (and frees its exclusive resources) before the winner's OnEnabled.
		for (size_t i = 0; i < impl->records.size(); ++i) {
			auto& record = impl->records[i];
			if (record.state == ModState::Active && !desired[i])
				impl->Deactivate(record, /*suppressed=*/rawEnabled[i] != 0);
		}

		// Pass 2: activate winners and tick them.
		for (size_t i = 0; i < impl->records.size(); ++i) {
			auto& record = impl->records[i];
			if (!desired[i])
				continue;
			if (record.state != ModState::Active)
				impl->Activate(record);
			if (record.state == ModState::Active)
				impl->ReconcileSongAndTick(record, phase);
		}
	}

	void ModRegistry::Shutdown() {
		for (auto& record : impl->records) {
			if (record.state == ModState::Registered)
				continue;

			if (record.state == ModState::Active)
				impl->Revert(record);
			impl->Invoke(record, &IMod::OnShutdown, "OnShutdown");
		}

		impl->records.clear();
	}

	ModRegistry& Registry() {
		static ModRegistry instance;
		return instance;
	}
}
