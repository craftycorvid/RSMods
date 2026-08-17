#include "ModRegistry.hpp"

#include <algorithm>
#include <chrono>
#include <deque>
#include <exception>
#include <iomanip>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../Log.hpp"
#include "ConflictResolver.hpp"
#include "HostHooks.hpp"
#include "MainThreadInbox.hpp"
#include "ModContext.hpp"

namespace Framework {
	PendingRegistration* g_modPendingHead = nullptr;

	// Registered initializes into Inactive; resolution moves Inactive <-> Active.
	// Active may park in Deactivating until render callbacks quiesce. Faulted is terminal.
	// The complete lifecycle contract and transition diagram live in README.md.
	enum class ModState {
		Registered,
		Inactive,
		Active,
		Deactivating,
		Faulted,
	};

	enum class DeactivationReason {
		Disabled,
		Suppressed,
		Faulted,
	};

	struct ModRegistry::Impl {
		using Hook = void (IMod::*)(ModContext&);

		// One flag per record, positionally indexed alongside `records` and `exclusiveResourcesByMod`.
		// vector<char> rather than vector<bool> so elements are addressable, plain bytes.
		using ActivationMask = std::vector<char>;

		struct PendingTeardown {
			DeactivationReason reason;

			ModState TargetState() const {
				return reason == DeactivationReason::Faulted
					? ModState::Faulted
					: ModState::Inactive;
			}

			const char* LogOutcome() const {
				switch (reason) {
				case DeactivationReason::Disabled:
					return " disabled";
				case DeactivationReason::Suppressed:
					return " suppressed by a higher-priority conflicting mod";
				case DeactivationReason::Faulted:
					return " faulted";
				}

				return " deactivated";
			}
		};

		struct Record {
			std::unique_ptr<IMod> mod;
			ModState state = ModState::Registered;
			bool inSong = false;
			std::optional<PendingTeardown> pendingTeardown;
		};

		bool Invoke(Record& record, Hook hook, const char* where) {
			ctx.currentMod = record.mod.get();

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

		bool IsRequestedSafe(Record& record) {
			ctx.currentMod = record.mod.get();

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
			record.pendingTeardown.reset();
			Hooks::Render().RemoveMod(record.mod.get());
			Commands().RemoveMod(record.mod.get());
		}

		// Best-effort revert of live game state before a mod leaves Active.
		void Revert(Record& record) {
			if (record.inSong) {
				Invoke(record, &IMod::OnSongExit, "OnSongExit");
				record.inSong = false;
			}

			Invoke(record, &IMod::OnDisabled, "OnDisabled");
		}

		void FinishTeardown(Record& record) {
			const PendingTeardown teardown = *record.pendingTeardown;

			Revert(record);
			if (teardown.TargetState() == ModState::Faulted) {
				Hooks::Render().RemoveMod(record.mod.get());
				Commands().RemoveMod(record.mod.get());
			}

			record.state = teardown.TargetState();
			record.pendingTeardown.reset();
			LOG_INFO("[Framework] " << record.mod->Id() << teardown.LogOutcome() << std::endl);
		}

		// Leaves Active. If render callbacks are still in flight, parks in Deactivating and
		// defers the OnDisabled revert to a later tick (never frees state under a live callback).
		void BeginTeardown(Record& record, DeactivationReason reason) {
			Hooks::Render().SetModActive(record.mod.get(), false);
			Commands().SetModActive(record.mod.get(), false);
			if (reason == DeactivationReason::Faulted) {
				Commands().SetModInitialized(record.mod.get(), false);
			}

			record.pendingTeardown = PendingTeardown{ reason };

			if (Hooks::Render().IsModQuiescent(record.mod.get())) {
				FinishTeardown(record);
			}
			else {
				record.state = ModState::Deactivating;
				LOG_INFO("[Framework] " << record.mod->Id() << " deactivating; deferring teardown until render callbacks quiesce" << std::endl);
			}
		}

		void RetryPendingDeactivations() {
			for (auto& record : records) {
				if (record.state == ModState::Deactivating && Hooks::Render().IsModQuiescent(record.mod.get())) {
					FinishTeardown(record);
				}
			}
		}

		// Escalate an already-running mod toward Faulted, respecting an in-flight teardown.
		// Redirects a pending deactivation to Faulted rather than starting a second teardown.
		void EscalateToFaulted(Record& record) {
			if (record.state == ModState::Active) {
				BeginTeardown(record, DeactivationReason::Faulted);
			}
			else if (record.state == ModState::Deactivating) {
				record.pendingTeardown = PendingTeardown{ DeactivationReason::Faulted };
				Commands().SetModInitialized(record.mod.get(), false);
			}
		}

		void HandleRenderFaults() {
			for (const IMod* mod : Hooks::Render().TakeFaultedMods()) {
				if (Record* record = FindByMod(mod)) EscalateToFaulted(*record);
			}
		}

		void HandleCommandFaults() {
			for (const IMod* mod : Commands().TakeFaultedMods()) {
				Record* record = FindByMod(mod);
				if (!record) continue;

				// A binding can fault while the mod is only Inactive (initialized, not active),
				// which the render path never sees; tear it down straight away.
				if (record->state == ModState::Inactive) {
					Fault(*record, "threw in a command");
				}
				else {
					EscalateToFaulted(*record);
				}
			}
		}

		Record* FindByMod(const IMod* mod) {
			for (auto& record : records) {
				if (record.mod.get() == mod) {
					return &record;
				}
			}

			return nullptr;
		}

		void Activate(Record& record) {
			if (!Invoke(record, &IMod::OnEnabled, "OnEnabled")) {
				Fault(record, "threw in OnEnabled");
				return;
			}

			record.state = ModState::Active;
			Hooks::Render().SetModActive(record.mod.get(), true);
			Commands().SetModActive(record.mod.get(), true);

			LOG_INFO("[Framework] " << record.mod->Id() << " activated" << std::endl);
		}

		// Runs an active-only hook; on throw, tear down (best-effort revert) and fault immediately.
		bool InvokeActive(Record& record, Hook hook, const char* where) {
			if (Invoke(record, hook, where)) {
				return true;
			}

			BeginTeardown(record, DeactivationReason::Faulted);
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
			auto batch = Inbox().DrainSettings();
			if (batch.empty()) return;

			for (auto& apply : batch) {
				try {
					apply();
				}
				catch (...) {
					LOG_ERROR("[Framework] a queued settings update threw while applying" << std::endl);
				}
			}

			// Only mods that are settled Inactive/Active get notified. A Deactivating mod still
			// has render callbacks in flight and its teardown revert is pending, so delivering
			// OnSettingsChanged there would touch state that is about to be freed (race/UAF).
			// It receives fresh settings when it next reaches Active (Activate -> OnEnabled).
			for (auto& record : records) {
				if (IsResolutionEligible(record.state)) {
					Invoke(record, &IMod::OnSettingsChanged, "OnSettingsChanged");
				}
			}

			Commands().RefreshDiagnostics();
		}

		bool IsResolutionEligible(ModState state) const {
			return state == ModState::Inactive || state == ModState::Active;
		}

		void BuildResourceIndexIfNeeded() {
			if (!resourceIndexDirty) return;

			exclusiveResourcesByMod.assign(records.size(), {});

			for (size_t i = 0; i < records.size(); ++i) {
				for (std::string_view resource : records[i].mod->ClaimsExclusive()) {
					auto& resources = exclusiveResourcesByMod[i];
					const std::string owned(resource);

					if (std::find(resources.begin(), resources.end(), owned) == resources.end()) {
						resources.push_back(owned);
					}
				}
			}

			resourceIndexDirty = false;
		}

		ActivationMask ComputeRequestedActive() {
			ActivationMask requestedActive(records.size(), 0);

			for (size_t i = 0; i < records.size(); ++i) {
				if (IsResolutionEligible(records[i].state)) {
					requestedActive[i] = IsRequestedSafe(records[i]);
				}
			}

			return requestedActive;
		}

		ActivationMask SelectActiveMods(const ActivationMask& requestedActive) {
			std::unordered_set<std::string_view> reservedByDeactivating;

			for (size_t i = 0; i < records.size(); ++i) {
				if (records[i].state == ModState::Deactivating) {
					for (const std::string& resource : exclusiveResourcesByMod[i]) {
						reservedByDeactivating.insert(resource);
					}
				}
			}

			std::vector<Resolver::Candidate> candidates(records.size());
			for (size_t i = 0; i < records.size(); ++i) {
				Resolver::Candidate& candidate = candidates[i];
				candidate.id = records[i].mod->Id();
				candidate.priority = records[i].mod->Priority();
				candidate.exclusiveResources = &exclusiveResourcesByMod[i];
				candidate.requested = requestedActive[i]
					&& IsResolutionEligible(records[i].state)
					&& std::none_of(exclusiveResourcesByMod[i].begin(), exclusiveResourcesByMod[i].end(),
						[&](const std::string& resource) {
							return reservedByDeactivating.count(resource) > 0;
						});
			}

			return Resolver::Resolve(candidates);
		}

		// Begin teardown of every Active mod the resolver did not select. Returns true if any
		// parked in Deactivating (still reserving resources), so the caller can re-resolve.
		bool BeginOutgoingTeardowns(const ActivationMask& requestedActive, const ActivationMask& selectedActive) {

			bool teardownDeferred = false;

			for (size_t i = 0; i < records.size(); ++i) {
				auto& record = records[i];

				if (record.state == ModState::Active && !selectedActive[i]) {
					const DeactivationReason reason = requestedActive[i]
						? DeactivationReason::Suppressed
						: DeactivationReason::Disabled;
					BeginTeardown(record, reason);
					teardownDeferred |= record.state == ModState::Deactivating;
				}
			}

			return teardownDeferred;
		}

		void ActivateAndTickSelected(const ActivationMask& selectedActive, GamePhase phase) {
			for (size_t i = 0; i < records.size(); ++i) {
				auto& record = records[i];

				if (!selectedActive[i]) continue;

				if (record.state != ModState::Active) Activate(record);
				if (record.state == ModState::Active) ReconcileSongAndTick(record, phase);
			}
		}

		std::vector<Record> records;
		ModContext ctx;
		bool resourceIndexDirty = false;
		std::vector<std::vector<std::string>> exclusiveResourcesByMod;
	};

	ModRegistry::ModRegistry() : impl(std::make_unique<Impl>()) {}
	ModRegistry::~ModRegistry() = default;

	void ModRegistry::InstantiatePending() {
		for (PendingRegistration* pending = g_modPendingHead; pending; pending = pending->next) {
			if (pending->factory) {
				Register(pending->factory());
			}
		}
	}

	void ModRegistry::Register(std::unique_ptr<IMod> mod) {
		const std::string_view id = mod->Id();

		for (const auto& record : impl->records) {
			if (record.mod->Id() == id) {
				LOG_ERROR("[Framework] Duplicate mod Id '" << id << "' - registration rejected" << std::endl);
				return;
			}
		}

		LOG_INFO("[Framework] Registered mod: " << id << std::endl);
		impl->records.push_back(Impl::Record{ std::move(mod) });
		impl->resourceIndexDirty = true;
	}

	void ModRegistry::DispatchInitialize() {
		for (auto& record : impl->records) {
			if (record.state != ModState::Registered) continue;

			if (impl->Invoke(record, &IMod::OnInitialize, "OnInitialize")) {
				record.state = ModState::Inactive;
				Commands().SetModInitialized(record.mod.get(), true);
			}
			else {
				impl->Fault(record, "threw in OnInitialize");
			}
		}

		Commands().RefreshDiagnostics();
	}

	void ModRegistry::DispatchCommands(GamePhase phase, bool gameLoaded) {
		impl->ctx.phase = phase;

		// Always drain the inbox so startup input isn't replayed later; the router discards it
		// when the game isn't loaded yet.
		const auto events = Inbox().DrainKeyEvents();
		Commands().DispatchPending(impl->ctx, events, gameLoaded);
		impl->HandleCommandFaults();
	}

	void ModRegistry::EnqueueSettingsUpdate(std::function<void()> apply) {
		Inbox().PostSettingsUpdate(std::move(apply));
	}

	void ModRegistry::Tick(GamePhase phase) {
		impl->ctx.phase = phase;

		impl->HandleRenderFaults();
		impl->HandleCommandFaults();
		impl->RetryPendingDeactivations();
		impl->DrainSettings();
		impl->BuildResourceIndexIfNeeded();

		const auto requestedActive = impl->ComputeRequestedActive();
		auto selectedActive = impl->SelectActiveMods(requestedActive);

		// Outgoing teardowns run before any activation so a replacement cannot acquire an
		// exclusive resource the outgoing (still-reserving) mod hasn't released. If a teardown
		// had to park in Deactivating, re-resolve so winners see the resources it still holds.
		if (impl->BeginOutgoingTeardowns(requestedActive, selectedActive)) {
			selectedActive = impl->SelectActiveMods(requestedActive);
		}

		impl->ActivateAndTickSelected(selectedActive, phase);
	}

	void ModRegistry::Shutdown() {
		for (auto& record : impl->records) {
			if (record.state == ModState::Registered)
				continue;

			if (record.state == ModState::Active || record.state == ModState::Deactivating) {
				Hooks::Render().SetModActive(record.mod.get(), false);
				Commands().SetModActive(record.mod.get(), false);

				while (!Hooks::Render().IsModQuiescent(record.mod.get())) {
					std::this_thread::sleep_for(std::chrono::milliseconds(5));
				}

				impl->Revert(record);
			}

			impl->Invoke(record, &IMod::OnShutdown, "OnShutdown");
			Hooks::Render().RemoveMod(record.mod.get());
			Commands().RemoveMod(record.mod.get());
		}

		impl->records.clear();
	}

	ModRegistry& Registry() {
		static ModRegistry instance;
		return instance;
	}
}
