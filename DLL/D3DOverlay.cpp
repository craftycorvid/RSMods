#include "stdafx.h"
#include "D3DOverlay.hpp"

/// <returns>Size of Rocksmith Window</returns>
Resolution GameOverlay::GetWindowSize() {
	RECT windowSize;

	Resolution currentSize;
	if (GetWindowRect(FindWindow(nullptr, L"Rocksmith 2014"), &windowSize))
	{
		currentSize.width = windowSize.right - windowSize.left;
		currentSize.height = windowSize.bottom - windowSize.top;
	}

	return currentSize;
}

/// <summary>
/// Draw text on screen
/// </summary>
/// <param name="textToDraw"> - What text should be written?</param>
/// <param name="textColorHex"> - What color? Given in hex in the AA,RR,GG,BB format.</param>
/// <param name="topLeftX"> - top LEFT of textbox</param>
/// <param name="topLeftY"> - TOP left of textbox</param>
/// <param name="bottomRightX"> - bottom RIGHT of textbox</param>
/// <param name="bottomRightY"> - BOTTOM right of textbox</param>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="setFontSize"> - Override font size</param>
/// <param name="format"> - DrawText format</param>
void GameOverlay::DX9DrawText(const std::string& textToDraw, int textColorHex, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, LPDIRECT3DDEVICE9 pDevice, Resolution setFontSize, DWORD format)
{
	Resolution windowSize = GameOverlay::GetWindowSize();

	bool useInputFontSize = (setFontSize.width != 0 && setFontSize.height != 0);

	int targetW = useInputFontSize ? setFontSize.width : (windowSize.width / 96);
	int targetH = useInputFontSize ? setFontSize.height : (windowSize.height / 72);

	const std::string face = Settings::ReturnSettingValue("OnScreenFont");
	FontKey key = FontKey::Make(face, targetH, targetW, FW_NORMAL, false);

	RECT TextRectangle{ topLeftX, topLeftY, bottomRightX, bottomRightY }; // Left, Top, Right, Bottom

	CComPtr<ID3DXFont> font;
	if (!fontCache.Get(pDevice, key, font)) {
		LOG_ERROR("Could not acquire required font.");
		return;
	}

	// Preload And Draw The Text (Supposed to reduce the performance hit (It's D3D/DX9 but still good practice))
	font->PreloadTextA(textToDraw.c_str(), textToDraw.length());
	font->DrawTextA(nullptr, textToDraw.c_str(), -1, &TextRectangle, format, textColorHex);
}

void GameOverlay::DisplayMixer() {
	// Display the whole mixer if displayMixer is true
	if (Settings::ReturnSettingValue("VolumeControlEnabled") == "on" && displayMixer) {

		float offset = 0;
		for (int volumeIndex = 0; volumeIndex < mixerInternalNames.size(); ++volumeIndex) {

			float volume = 0;
			RTPCValue_type type = RTPCValue_GameObject;
			Wwise::SoundEngine::Query::GetRTPCValue(mixerInternalNames[volumeIndex].c_str(), AK_INVALID_GAME_OBJECT, &volume, &type);

			DX9DrawText(
				drawMixerTextName[volumeIndex] + std::to_string(static_cast<int>(volume)) + "%",
				whiteText,
				static_cast<int>(WindowSize.width / 96.0f),  // 20 pixels from left in 1920x1080 resolution
				static_cast<int>(WindowSize.height / 54.0f + offset), // 20 pixels from top (plus an offset to display multiple values)
				static_cast<int>(WindowSize.width / 19.2f),  // 120 pixels from left
				static_cast<int>(WindowSize.height / 16.0f), // 120 pixels from top
				pDevice);

			// Adjust the offset to display the next value
			offset += WindowSize.height / 54.0f;
		}
	}
	// Display just the current volume based on context (This will display the last volume that was adjusted for a few seconds after adjusting it)
	else if (Settings::ReturnSettingValue("VolumeControlEnabled") == "on" && displayCurrentVolume) {
		float volume = 0;
		RTPCValue_type type = RTPCValue_GameObject;
		Wwise::SoundEngine::Query::GetRTPCValue(mixerInternalNames[currentVolumeIndex].c_str(), AK_INVALID_GAME_OBJECT, &volume, &type);

		DX9DrawText(
			drawMixerTextName[currentVolumeIndex] + std::to_string(static_cast<int>(volume)) + "%",
			whiteText,
			static_cast<int>(WindowSize.width / 96.0f),  // 20 pixels from left in 1920x1080 resolution
			static_cast<int>(WindowSize.height / 54.0f), // 20 pixels from top 
			static_cast<int>(WindowSize.width / 19.2f),  // 120 pixels from left
			static_cast<int>(WindowSize.height / 16.0f), // 120 pixels from top
			pDevice);
	}
}

void GameOverlay::DisplaySongTimer()
{
	if (D3DHooks::showSongTimerOnScreen && SongTimer::SongTimer() != 0.f) {
		DX9DrawText(
			D3DHooks::ConvertFloatTimeToStringTime(SongTimer::SongTimer()),
			whiteText,
			static_cast<int>(WindowSize.width - WindowSize.width / 16.0f), // 120 pixels left from right edge in 1920x1080 resolution
			static_cast<int>(WindowSize.height / 54.0f),                   // 20 pixels from top
			static_cast<int>(WindowSize.width - WindowSize.width / 96.0f), // 20 left from right edge
			static_cast<int>(WindowSize.height / 16.0f),                   // 120 pixels from top
			pDevice,
			{ NULL, NULL },
			DT_RIGHT | DT_NOCLIP);
	}
}

void GameOverlay::DisplayCurrentNote()
{
	if (Settings::ReturnSettingValue("ShowCurrentNoteOnScreen") == "on" && GuitarSpeak::GetCurrentNoteName() != (std::string)"") {

		if (GameState::IsInSong()) {
			DX9DrawText(
				GuitarSpeak::GetCurrentNoteName(),
				whiteText,
				static_cast<int>(WindowSize.width / 5.5),		// 349 pixels left of the center in 1920x1080 resolution.
				static_cast<int>(WindowSize.height / 1.75),	// 617 pixels from the top
				static_cast<int>(WindowSize.width / 5.75),		// 334 pixels right of center
				static_cast<int>(WindowSize.height / 8),		// 135 pixels from the top
				pDevice);
		}
		else { // Show outside of the song at the top of the screen.
			DX9DrawText(
				"Current Note: " + GuitarSpeak::GetCurrentNoteName(),
				whiteText,
				static_cast<int>(WindowSize.width / 3.87),		// 496 pixels left of the center in 1920x1080 resolution
				static_cast<int>(WindowSize.height / 30.85),	// 35 pixels from the top
				static_cast<int>(WindowSize.width / 4),		// 480 pixel right of the center
				static_cast<int>(WindowSize.height / 8),		// 135 pixels from the top
				pDevice);
		}
	}
}

void GameOverlay::DisplayRiffRepeaterOverHundredPercentSpeed()
{
	if (Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && RiffRepeater::loggedCurrentSongID &&
		(GameState::Menus::IsInModesWithAllowedFastRiffRepeater() || GameState::Menus::IsOnScoreScreens()) || RiffRepeater::currentlyEnabled_Above100) {
		realSongSpeed = RiffRepeater::GetSpeed(true); // While this should almost always be the same value, the user might enable riff repeater, which could cause this number to be wrong.

		DX9DrawText(
			"Song Speed: " + std::to_string(static_cast<int>(roundf(realSongSpeed))) + "%",
			whiteText,
			static_cast<int>(WindowSize.width / 2.0f - WindowSize.width / 38.4f), // 50 pixels left of center in 1920x1080 resolution
			static_cast<int>(WindowSize.height / 54.0f),                          // 20 pixels from top
			static_cast<int>(WindowSize.width / 2.0f + WindowSize.width / 38.4f), // 50 pixels right of center
			static_cast<int>(WindowSize.height / 16.0f),                          // 120 pixels from top
			pDevice,
			{ NULL, NULL },
			DT_CENTER | DT_NOCLIP);
	}
}

void GameOverlay::DisplayCurrentTuningForAutoTune()
{
	if (Settings::ReturnSettingValue("AutoTuneForSong") == "on" && Settings::GetKeyBind("TuningOffsetKey") != NULL && GameState::Menus::IsInTuningMenus()) {
		DX9DrawText(
			"Auto Tune For: " + Midi::GetTuningOffsetName(Midi::tuningOffset),
			whiteText,
			static_cast<int>(WindowSize.width / 5.5),		// 349 pixels left of the center in 1920x1080 resolution
			static_cast<int>(WindowSize.height / 30.85),	// 35 pixels from the top
			static_cast<int>(WindowSize.width / 5.65),		// 339 pixels right of center
			static_cast<int>(WindowSize.height / 8),		// 135 pixels from the top
			pDevice);
	}
}

void GameOverlay::DisplayLoopStartEndTimes(float loopStart, float loopEnd)
{
	DX9DrawText(
		"Loop: " + D3DHooks::ConvertFloatTimeToStringTime(loopStart) + " - " + D3DHooks::ConvertFloatTimeToStringTime(loopEnd),
		whiteText,
		static_cast<int>(WindowSize.width / 2.0f - WindowSize.width / 38.4f), // 50 pixels left of center in 1920x1080 resolution
		static_cast<int>(WindowSize.height / 21.6f),                          // 50 pixels from top
		static_cast<int>(WindowSize.width / 2.0f + WindowSize.width / 38.4f), // 50 pixels right of center
		static_cast<int>(WindowSize.height / 7.2f),                           // 150 pixels from top
		pDevice,
		{ NULL, NULL },
		DT_CENTER | DT_NOCLIP);
}

void HandleLooping() {
	if (Settings::ReturnSettingValue("AllowLooping") == "on" && (Keybindings::loopStart != NULL || Keybindings::loopEnd != NULL)) {
		// Only enable looping in learn a song modes (learn a song & non-stop play)
		if (GameState::Menus::IsInLearnASongModes()) {
			GameOverlay::DisplayLoopStartEndTimes(Keybindings::loopStart, Keybindings::loopEnd);

			// Prevent the user from creating a loop that starts at a negative timestamp.
			if ((Settings::GetModSetting("LoopingLeadUp") / 1000.f) >= Keybindings::loopStart) {
				Keybindings::roughLoopStart = 0.f;
			}
			else {
				Keybindings::roughLoopStart = Keybindings::loopStart - (Settings::GetModSetting("LoopingLeadUp") / 1000.f);
			}

			// If we are paused, reset the grey note timer.
			if (GameState::Menus::IsInLearnASongPauseModes()) {
				// Resets grey note timer to loopStart. This makes it so notes in the loop are not deactivated.
				// Deactivated notes are greyed out, and do not register with note detection.
				// As an added bonus the game also automatically adds a bit of lead time so the player has some time to prepare.
				if (SongTimer::GetGreyNoteTimer() != Keybindings::loopStart) {
					SongTimer::SetGreyNoteTimer(Keybindings::loopStart);
				}
			}

			// If not paused AND we are at the end of the loop, seek to the start of the loop.
			else if (Keybindings::loopStart != NULL && Keybindings::loopEnd != NULL && (SongTimer::SongTimer() >= Keybindings::loopEnd)) {
				Wwise::SoundEngine::SeekOnEvent(std::string("Play_" + GameState::GetSongKey()).c_str(), 0x1234, (AkTimeMs)(Keybindings::roughLoopStart * 1000), false);
			}
		}
		// Difference between learnASongModes & fastRRModes is the inclusion of RR. This means that this check is only gets the RR menus.
		else if (GameState::Menus::IsInModesWithAllowedFastRiffRepeater()) {
			// Reset loopStart and loopEnd to NULL as the user wants to do a loop with RR, or is changing some settings.
			Keybindings::loopStart = NULL;
			Keybindings::loopEnd = NULL;
		}
	}

}

void GameOverlay::RenderOverlay(IDirect3DDevice9* pDevice) {
	// Draw text on screen
	// NOTE: NEVER USE SET VALUES. Always do division of WindowSize width AND heigh so every resolution should have the text in around the same spot.
	if (GameState::GameLoaded) {
		Resolution windowSize = GameOverlay::GetWindowSize();

		WindowSize = GetWindowSize();
		pDevice = device;

		GameOverlay::DisplayMixer();
		GameOverlay::DisplaySongTimer();
		GameOverlay::DisplayRiffRepeaterOverHundredPercentSpeed();
		GameOverlay::DisplayCurrentNote();
		GameOverlay::DisplayCurrentTuningForAutoTune();

		HandleLooping();
	}
}