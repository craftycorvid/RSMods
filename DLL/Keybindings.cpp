#include "stdafx.h"
#include "Keybindings.hpp"

namespace {
	Framework::KeyEvent CaptureKeyEvent(WPARAM keyPressed, LPARAM lParam, Framework::KeyEdge edge) {
		Framework::KeyEvent event;
		event.virtualKey = static_cast<std::uint32_t>(keyPressed);
		event.edge = edge;
		event.control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		event.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
		event.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
		event.repeat = edge == Framework::KeyEdge::Down && (lParam & (1LL << 30)) != 0;
		return event;
	}
}

namespace Keybindings {
	void HandleKeyUp(WPARAM keyPressed, LPARAM lParam) {
		Framework::Commands().Enqueue(CaptureKeyEvent(
			keyPressed, lParam, Framework::KeyEdge::Up));
	void HandleTuningOffset()
	{
		bool isCtrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;

		Midi::tuningOffset += isCtrlPressed ? -1 : 1;
		Midi::tuningOffset = std::clamp(Midi::tuningOffset, -3, 12);

		LOG_INFO("Triggered Mod Setting: Tuning Offset is now set to " << Midi::tuningOffset << std::endl);
	}

	void HandleKeyDown(WPARAM keyPressed, LPARAM lParam) {
		Framework::Commands().Enqueue(CaptureKeyEvent(
			keyPressed, lParam, Framework::KeyEdge::Down));
	}

	void InitializeCommands() {
		auto& commands = Framework::Commands();
		commands.SetKeyResolver([](std::string_view setting) {
			return Settings::GetKeyBind(std::string(setting));
		});
		commands.BindKey(nullptr, "ReloadSettings", 'A', Framework::KeyEdge::Up,
			Framework::Availability::Initialized,
			[](Framework::ModContext&, const Framework::KeyEvent&) {
				Settings::UpdateSettings();
				Framework::Commands().RefreshDiagnostics();
				LOG_INFO("Triggered Setting Update" << std::endl);
			},
			[](const Framework::ModContext&, const Framework::KeyEvent& event) {
				return event.control;
			});
		commands.BindKey(nullptr, "ToggleDebugMenu", VK_BACK, Framework::KeyEdge::Up,
			Framework::Availability::Initialized,
			[](Framework::ModContext&, const Framework::KeyEvent&) {
				Menu::menuEnabled = !Menu::menuEnabled;
			},
			[](const Framework::ModContext&, const Framework::KeyEvent&) {
				return D3DHooks::debug;
			});
	}

	void UpdateSettingsOnGUIChange(LPARAM lParam) {
		auto pcds = reinterpret_cast<COPYDATASTRUCT*>(lParam);
		if (pcds->dwData != 1) return;

		std::string currMsg = static_cast<char*>(pcds->lpData);
		LOG_INFO(currMsg << std::endl);

			if (Contains(currMsg, "update")) {
				if (Contains(currMsg, "all"))
					Framework::Registry().EnqueueSettingsUpdate([] { Settings::UpdateSettings(); });
				else
					Framework::Registry().EnqueueSettingsUpdate([currMsg] { Settings::ParseSettingUpdate(currMsg); });
			}
			else if (Contains(currMsg, "WwiseEvent")) {
				auto msgParts = Settings::SplitByWhitespace(currMsg);

				if (msgParts.size() == 2)
					VoiceOverControl::PlayVoiceOver(msgParts[1]);
			}
			else if (Contains(currMsg, "TurboSpeed")) // Deprecated - use CrowdControl
			{
				if (Contains(currMsg, "enable"))
					RiffRepeater::EnableTimeStretch();
				else
					RiffRepeater::DisableTimeStretch();
			}
			else if (Contains(currMsg, "enable"))
				Twitch::effectQueue.push_back(currMsg);
			else if (Contains(currMsg, "Reconnect"))
				CrowdControl::StartServerLoop();
		}
	}

	void DispatchCommand(WPARAM keyPressed, const std::map<std::string, ModCommand, std::less<>>& commands)
	{
		for (const auto& [key, value] : commands) {

			if (keyPressed == Settings::GetKeyBind(key)) {
				if (!value.condition()) return;

				value.action();

				if (!value.logMessage.empty()) {
					LOG_INFO("Triggered: " << value.logMessage << std::endl);
				}

				return;
			}
		}
	}

	void InitializeCommands()
	{
		keyUpCommands["ToggleLoftKey"] =
		{
			[] { return Settings::ReturnSettingValue("ToggleLoftEnabled") == "on"; },
			[] { Loft::ToggleLoft(); },
			"Toggle Loft"
		};
		keyUpCommands["ShowSongTimerKey"] =
		{
			[] { return Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on"; },
			[] { D3DHooks::showSongTimerOnScreen = !D3DHooks::showSongTimerOnScreen; },
			"Show Song Timer"
		};
		keyUpCommands["ForceReEnumerationKey"] =
		{
			[] { return Settings::ReturnSettingValue("ForceReEnumerationEnabled") == "manual"; },
			[] { Enumeration::ForceEnumeration(); },
			"Force Enumeration"
		};
		keyUpCommands["RainbowStringsKey"] =
		{
			[] { return Settings::ReturnSettingValue("RainbowStringsEnabled") == "on"; },
			[] { ERMode::ToggleRainbowMode();
				 if (!ERMode::RainbowEnabled) ERMode::ResetAllStrings(); },
			"Rainbow Strings"
		};
		keyUpCommands["TuningOffsetKey"] =
		{
			[] { return Settings::ReturnSettingValue("AutoTuneForSong") == "on"; },
			HandleTuningOffset,
			""
		};
		keyUpCommands["RewindKey"] =
		{
			[] { return Settings::ReturnSettingValue("AllowRewind") == "on" && GameState::Menus::IsInLASPlayingModes(); },
			HandleRewind,
			""
		};
		keyDownCommands["LoopStartKey"] =
		{
			[] { return Settings::ReturnSettingValue("AllowLooping") == "on" && GameState::Menus::IsInModesWithAllowedFastRiffRepeater(); },
			SetLoopStartingPoint,
			"Loop Start Point Set"
		};
		keyDownCommands["LoopEndKey"] =
		{
			[] { return Settings::ReturnSettingValue("AllowLooping") == "on" && GameState::Menus::IsInModesWithAllowedFastRiffRepeater(); },
			SetLoopEndingPoint,
			"Loop End Point Set"
		};
		keyUpCommands["RemoveLyricsKey"] =
		{
			[] { return Settings::ReturnSettingValue("RemoveLyricsWhen") == "manual"; },
			[] { D3DHooks::RemoveLyrics = !D3DHooks::RemoveLyrics; },
			"Remove Lyrics"
		};
		keyUpCommands["ToggleExtendedRangeKey"] =
		{
			[] { return true; },
			[] { ERMode::UseERExclusivelyInThisSong = !ERMode::UseERExclusivelyInThisSong; GameState::ToggleCB(ERMode::UseERExclusivelyInThisSong); },
			"Toggle Extended Range"
		};
		keyUpCommands["MutePlayer1Key"] =
		{
			[] { return true; },
			[] { HandleMutePlayer1(); },
			"Mute Player 1"
		};
		keyUpCommands["MutePlayer2Key"] =
		{
			[] { return true; },
			[] { HandleMutePlayer2(); },
			"Mute Player 2"
		};
		keyUpCommands["DisplayMixerKey"] =
		{
			[] { return true; },
			[] { GameOverlay::displayMixer = false; }, // Hide the mixer if it is not actively being pressed
			""
		};
		keyUpCommands["ChangedSelectedVolumeKey"] =
		{
			[] { return Settings::ReturnSettingValue("VolumeControlEnabled") == "on"; },
			[] {
					GameOverlay::currentVolumeIndex++;
					if (GameOverlay::currentVolumeIndex > (GameOverlay::mixerInternalNames.size() - 1)) // There are only so many values we know we can edit, so loop back.
						GameOverlay::currentVolumeIndex = 0;},
			""
		};

		keyDownCommands["RRSpeedKey"] =
		{
			[] { return Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && GameState::Menus::IsInModesWithAllowedFastRiffRepeater() && RiffRepeater::loggedCurrentSongID; },
			HandleRRSpeed,
			""
		};
		keyDownCommands["DisplayMixerKey"] =
		{
			[] { return Settings::ReturnSettingValue("VolumeControlEnabled") == "on"; },
			[] { GameOverlay::displayMixer = true; },
			"Display Mixer"
		};
	}
}
