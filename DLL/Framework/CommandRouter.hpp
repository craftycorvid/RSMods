#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "CommandTypes.hpp"

namespace Framework {
	class IMod;

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
