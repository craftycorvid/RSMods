#pragma once

#include "stdafx.h"
#include "Mods/GuitarSpeak.hpp"
#include "Mods/RiffRepeater.hpp"
#include "Mods/Midi.hpp"

namespace GameOverlay {
	Resolution GetWindowSize();
	void DX9DrawText(const std::string& textToDraw, int textColorHex, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, LPDIRECT3DDEVICE9 pDevice, Resolution setFontSize = { NULL, NULL }, DWORD format = DT_LEFT | DT_NOCLIP);

	inline HRESULT CustomDX9Font = NULL;
	inline ID3DXFont* DX9FontEncapsulation = NULL;
	inline int fontWidth = NULL, fontHeight = NULL;

	const int whiteText = 0xFFFFFFFF;

	inline Resolution WindowSize;
	inline IDirect3DDevice9* pDevice;
	void SetPDevice(IDirect3DDevice9* pDevice, Resolution windowSize);
	void DisplayMixer();
	void DisplaySongTimer();
	void DisplayCurrentNote();
	void DisplayRiffRepeaterOverHundredPercentSpeed();
	void DisplayCurrentTuningForAutoTune();
	void DisplayLoopStartEndTimes(float loopStart, float loopEnd);
	void RenderOverlay(IDirect3DDevice9* pDevice);

	inline std::vector<std::string> mixerInternalNames = { // Needs to be char* as that's what SetRTPCValue needs.
		{"Master_Volume"}, // Master Volume
		{"Mixer_Music"}, // Song Volume
		{"Mixer_Player1"}, // Player 1 Guitar & Bass (both are handled with this singular name)
		{"Mixer_Player2"}, // Player 2 Guitar & Bass (both are handled with this singular name)
		{"Mixer_Mic"}, // My Microphone Volume
		{"Mixer_VO"}, // Rocksmith Dad Voice Over
		{"Mixer_SFX"}, // Menu SFX Volume
	};

	inline std::vector<std::string> drawMixerTextName = {
		{"Master Volume: "},
		{"Song Volume: "},
		{"Player 1 Volume: "},
		{"Player 2 Volume: "},
		{"Microphone Volume: "},
		{"Voice-Over Volume: "},
		{"SFX Volume: "},
	};

	inline unsigned int currentVolumeIndex = 0; // Mixer volume to change. 0 - Master, 1 - Song, 2 - P1, 3 - P2, 4 - Mic, 5 - VO, 6 - SFX

	// Volume adjustment mod
	inline bool displayMixer = false;
	inline bool displayCurrentVolume = false;
	inline auto displayVolumeStartTime = std::chrono::steady_clock::time_point(); // Defaults to epoch time
}