#pragma once

#include <string>
#include <string_view>
#include <utility>

#include "GamePhase.hpp"
#include "HostHooks.hpp"
#include "../Settings.hpp"

namespace Framework {
	struct RenderBinder {
		Hooks::RenderHooks& hooks;
		const IMod* mod;

		void OnEndScene(Hooks::EndSceneFn fn) const { hooks.Subscribe(mod, std::move(fn)); }
	};

	// Internal per-hook context
	struct ModContext {
		GamePhase phase = GamePhase::Loading;
		const IMod* currentMod = nullptr; // Set by the registry before each hook call.

		RenderBinder Render() const { return { Hooks::Render(), currentMod }; } // Subscribe per-frame draw callbacks.

		bool IsOn(std::string_view key) const { return Settings::IsOn(std::string(key)); }
		bool IsOff(std::string_view key) const { return Settings::IsOff(std::string(key)); }
		std::string Value(std::string_view key) const { return Settings::ReturnSettingValue(std::string(key)); }
		int  Int(std::string_view key) const { return Settings::GetModSetting(std::string(key)); }

		Settings::When WhenSetting(std::string_view key) const { return Settings::GetWhen(std::string(key)); }
		Settings::StringColorMode ColorModeSetting(std::string_view key) const { return Settings::GetStringColorMode(std::string(key)); }
		Settings::NoteColorMode NoteColorModeSetting(std::string_view key) const { return Settings::GetNoteColorMode(std::string(key)); }
	};
}
