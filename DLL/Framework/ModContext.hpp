#pragma once

#include <string>
#include <string_view>

#include "GamePhase.hpp"
#include "../Settings.hpp"

struct GameLoopState;

namespace Framework {
	// Internal per-hook context
	struct ModContext {
		GamePhase phase = GamePhase::Loading;
		GameLoopState* loop = nullptr;   // Transitional: fields migrate into the mods that own them.

		bool IsOn(std::string_view key) const { return Settings::IsOn(std::string(key)); }
		bool IsOff(std::string_view key) const { return Settings::IsOff(std::string(key)); }
		std::string Value(std::string_view key) const { return Settings::ReturnSettingValue(std::string(key)); }
		int  Int(std::string_view key) const { return Settings::GetModSetting(std::string(key)); }

		Settings::When WhenSetting(std::string_view key) const { return Settings::GetWhen(std::string(key)); }
		Settings::StringColorMode ColorModeSetting(std::string_view key) const { return Settings::GetStringColorMode(std::string(key)); }
		Settings::NoteColorMode NoteColorModeSetting(std::string_view key) const { return Settings::GetNoteColorMode(std::string(key)); }
	};
}
