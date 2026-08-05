#pragma once

#include <Windows.h>
#include "Mods/VolumeControl.hpp"
#include "Framework/Framework.hpp"
#include "Menu.hpp"
#include "Mods/VoiceOverControl.hpp"
#include "Twitch.hpp"
#include "CC/ControlServer.hpp"

namespace Keybindings {
	void HandleKeyUp(WPARAM keyPressed, LPARAM lParam);
	void HandleKeyDown(WPARAM keyPressed, LPARAM lParam);

	void InitializeCommands();
	void UpdateSettingsOnGUIChange(LPARAM lParam);

	// Looping state is also consumed by the render overlay.
	inline float loopStart = NULL;
	inline float roughLoopStart = NULL;
	inline float loopEnd = NULL;
}
