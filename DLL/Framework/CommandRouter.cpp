#include "CommandRouter.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <exception>
#include <iomanip>
#include <mutex>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "../Log.hpp"
#include "CommandCollisionDiagnostics.hpp"
#include "ModContext.hpp"

namespace Framework {
	struct CommandRouter::Impl {
		enum class BindingKind {
			Setting,
			FixedKey,
		};

		struct Binding {
			const IMod* mod = nullptr;
			std::string commandName;
			std::string keySetting;
			std::optional<std::uint32_t> fixedVirtualKey;
			KeyEdge edge = KeyEdge::Up;
			Availability availability = Availability::Active;
			BindingKind kind = BindingKind::Setting;
			KeyAction action;
			KeyPredicate predicate;
			std::string logMessage;

			static Binding ForSetting(const IMod* mod, std::string keySetting, KeyEdge edge,
				Availability availability, KeyAction action,
				KeyPredicate predicate, std::string logMessage) {

				Binding binding;
				binding.mod = mod;
				binding.commandName = keySetting;
				binding.keySetting = std::move(keySetting);
				binding.edge = edge;
				binding.availability = availability;
				binding.action = std::move(action);
				binding.predicate = std::move(predicate);
				binding.logMessage = std::move(logMessage);
				
				return binding;
			}

			static Binding ForFixedKey(const IMod* mod, std::string name, std::uint32_t virtualKey,
				KeyEdge edge, Availability availability, KeyAction action,
				KeyPredicate predicate, std::string logMessage) {

				Binding binding;
				binding.mod = mod;
				binding.commandName = std::move(name);
				binding.fixedVirtualKey = virtualKey;
				binding.edge = edge;
				binding.availability = availability;
				binding.kind = BindingKind::FixedKey;
				binding.action = std::move(action);
				binding.predicate = std::move(predicate);
				binding.logMessage = std::move(logMessage);

				return binding;
			}

			bool operator==(const Binding& other) const {
				return std::tie(edge, kind, commandName, fixedVirtualKey) ==
					std::tie(other.edge, other.kind, other.commandName, other.fixedVirtualKey);
			}

			bool operator<(const Binding& other) const {
				return std::tie(edge, kind, commandName) < std::tie(other.edge, other.kind, other.commandName);
			}

			unsigned int ResolveKey(const KeyResolver& resolver) const {
				if (fixedVirtualKey) return *fixedVirtualKey;

				return resolver ? resolver(keySetting) : 0;
			}

			bool Matches(const KeyEvent& event, BindingKind requiredKind,
				const KeyResolver& resolver) const {

				return kind == requiredKind && edge == event.edge && ResolveKey(resolver) == event.virtualKey;
			}
		};

		struct ModCommandState {
			bool initialized = false;
			bool active = false;
			bool faulted = false;
		};

		struct RoutingSnapshot {
			std::vector<Binding> bindings;
			KeyResolver resolver;
		};

		inline static constexpr std::array<BindingKind, 2> DispatchOrder = {
			BindingKind::Setting,
			BindingKind::FixedKey,
		};

		void AddBinding(Binding binding) {
			if (std::find(bindings.begin(), bindings.end(), binding) != bindings.end()) {
				LOG_ERROR("[Framework] duplicate binding '" << binding.commandName << "' rejected" << std::endl);
				return;
			}

			bindings.push_back(std::move(binding));
			std::sort(bindings.begin(), bindings.end());
		}

		bool IsAvailable(const Binding& binding) const {
			if (!binding.mod) return true;

			std::lock_guard<std::mutex> lock(mutex);

			const auto mod = modStates.find(binding.mod);
			if (mod == modStates.end() || mod->second.faulted) return false;
			
			return binding.availability == Availability::Active
				? mod->second.active
				: mod->second.initialized;
		}

		const Binding* FindBinding(const RoutingSnapshot& routing, const KeyEvent& event, BindingKind kind) const {
			for (const Binding& binding : routing.bindings) {
				if (binding.Matches(event, kind, routing.resolver) && IsAvailable(binding)) {
					return &binding;
				}
			}

			return nullptr;
		}

		void InvokeBinding(const Binding& binding, ModContext& context, const KeyEvent& event) {
			context.currentMod = binding.mod;

			try {
				if (binding.predicate && !binding.predicate(context, event)) return;

				binding.action(context, event);
				if (!binding.logMessage.empty()) {
					LOG_INFO("Triggered: " << binding.logMessage << std::endl);
				}
			}
			catch (const std::exception& ex) {
				NoteFault(binding.mod, binding.commandName, ex.what());
			}
			catch (...) {
				NoteFault(binding.mod, binding.commandName, "threw an unknown exception");
			}
		}

		void DispatchEvent(ModContext& context, const KeyEvent& event, const RoutingSnapshot& routing) {
			for (BindingKind kind : DispatchOrder) {
				const Binding* binding = FindBinding(routing, event, kind);
				if (binding) InvokeBinding(*binding, context, event);
			}
		}

		RoutingSnapshot SnapshotRouting() const {
			std::lock_guard<std::mutex> lock(mutex);
			return { bindings, keyResolver };
		}

		std::vector<Detail::ResolvedCommandBinding> ResolveBindingsForDiagnostics(
			const RoutingSnapshot& routing) const {
			std::vector<Detail::ResolvedCommandBinding> descriptions;
			descriptions.reserve(routing.bindings.size());

			for (const auto& binding : routing.bindings) {
				const unsigned int key = binding.ResolveKey(routing.resolver);
				if (key == 0) continue;

				const auto kind = binding.kind == BindingKind::Setting
					? Detail::CommandBindingKind::Setting
					: Detail::CommandBindingKind::FixedKey;
				descriptions.push_back({ binding.edge, key, kind, binding.commandName });
			}

			return descriptions;
		}

		void NoteFault(const IMod* mod, std::string_view name, const char* detail) {
			if (!mod) {
				LOG_ERROR("[Framework] host binding '" << name << "' " << detail << std::endl);
				return;
			}

			std::lock_guard<std::mutex> lock(mutex);

			auto& state = modStates[mod];
			if (!state.faulted) {
				state.faulted = true;
				faultedMods.push_back(mod);
				LOG_ERROR("[Framework] binding '" << name << "' " << detail << "; its mod will be disabled" << std::endl);
			}
		}

		std::vector<Binding> bindings;
		std::unordered_map<const IMod*, ModCommandState> modStates;
		std::vector<const IMod*> faultedMods;
		KeyResolver keyResolver;
		Detail::CommandCollisionDiagnostics collisionDiagnostics;
		mutable std::mutex mutex;
	};

	CommandRouter::CommandRouter() : impl(std::make_unique<Impl>()) {}
	CommandRouter::~CommandRouter() = default;

	void CommandRouter::BindSetting(const IMod* mod, std::string keySetting, KeyEdge edge,
		Availability availability, KeyAction action, KeyPredicate predicate, std::string logMessage) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		impl->AddBinding(Impl::Binding::ForSetting(mod, std::move(keySetting), edge, availability,
			std::move(action), std::move(predicate), std::move(logMessage)));
	}

	void CommandRouter::BindKey(const IMod* mod, std::string name, std::uint32_t virtualKey,
		KeyEdge edge, Availability availability, KeyAction action,
		KeyPredicate predicate, std::string logMessage) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		
		impl->AddBinding(Impl::Binding::ForFixedKey(mod, std::move(name), virtualKey, edge,
			availability, std::move(action), std::move(predicate), std::move(logMessage)));
	}

	void CommandRouter::SetKeyResolver(KeyResolver resolver) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		
		impl->keyResolver = std::move(resolver);
	}

	void CommandRouter::SetModInitialized(const IMod* mod, bool initialized) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		auto& state = impl->modStates[mod];
		state.initialized = initialized;
		if (!initialized) state.active = false;
	}

	void CommandRouter::SetModActive(const IMod* mod, bool active) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		
		impl->modStates[mod].active = active;
	}

	void CommandRouter::RemoveMod(const IMod* mod) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		
		impl->bindings.erase(std::remove_if(impl->bindings.begin(), impl->bindings.end(),
			[mod](const Impl::Binding& binding) { return binding.mod == mod; }), impl->bindings.end());
		impl->modStates.erase(mod);
		impl->faultedMods.erase(std::remove(impl->faultedMods.begin(), impl->faultedMods.end(), mod),
			impl->faultedMods.end());
	}

	void CommandRouter::DispatchPending(ModContext& context, const std::deque<KeyEvent>& events, bool gameLoaded) {
		// Input received during startup is deliberately discarded (drained by the caller, not replayed).
		if (!gameLoaded) return;

		const auto routing = impl->SnapshotRouting();
		for (const KeyEvent& event : events) {
			impl->DispatchEvent(context, event, routing);
		}
	}

	std::vector<const IMod*> CommandRouter::TakeFaultedMods() {
		std::lock_guard<std::mutex> lock(impl->mutex);
		std::vector<const IMod*> mods;
		
		mods.swap(impl->faultedMods);

		return mods;
	}

	void CommandRouter::RefreshDiagnostics() {
		const auto routing = impl->SnapshotRouting();
		impl->collisionDiagnostics.Refresh(impl->ResolveBindingsForDiagnostics(routing));
	}

	CommandRouter& Commands() {
		static CommandRouter instance;
		
		return instance;
	}
}
