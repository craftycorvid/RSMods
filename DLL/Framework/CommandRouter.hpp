#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Framework {
	struct ModContext;
	class IMod;

	enum class KeyEdge {
		Down,
		Up,
	};

	enum class Availability {
		// Strictly ModState::Active. Deactivating mods are already unavailable.
		Active,
		// Available after successful initialization until fault/shutdown, even while disabled or suppressed.
		// Never use this for a command that mutates a resource returned by ClaimsExclusive().
		Initialized,
	};

	struct KeyEvent {
		std::uint32_t virtualKey = 0;
		KeyEdge edge = KeyEdge::Up;
		bool control = false;
		bool shift = false;
		bool alt = false;
		bool repeat = false;
	};

	using KeyPredicate = std::function<bool(const ModContext&, const KeyEvent&)>;
	using KeyAction = std::function<void(ModContext&, const KeyEvent&)>;
	using KeyResolver = std::function<unsigned int(std::string_view)>;

	class CommandRouter {
	public:
		CommandRouter();
		~CommandRouter();

		CommandRouter(const CommandRouter&) = delete;
		CommandRouter& operator=(const CommandRouter&) = delete;

		void BindSetting(const IMod* mod, std::string keySetting, KeyEdge edge,
			Availability availability, KeyAction action,
			KeyPredicate predicate = {}, std::string logMessage = {});
		void BindKey(const IMod* mod, std::string name, std::uint32_t virtualKey,
			KeyEdge edge, Availability availability, KeyAction action,
			KeyPredicate predicate = {}, std::string logMessage = {});
		void SetKeyResolver(KeyResolver resolver);

		void SetModInitialized(const IMod* mod, bool initialized);
		void SetModActive(const IMod* mod, bool active);
		void RemoveMod(const IMod* mod);

		void Enqueue(KeyEvent event);
		void Wake();
		void WaitUntil(std::chrono::steady_clock::time_point deadline);
		void DispatchPending(ModContext& context, bool gameLoaded);

		std::vector<const IMod*> TakeFaultedMods();
		void RefreshDiagnostics();

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

	CommandRouter& Commands();
}
