#include "stdafx.h"
#include "Main.hpp"

#if defined(_DEBUG) || defined(_WWISE_LOGS)
bool debug = true; // You ARE on a debug build.
#else
bool debug = false; // You are NOT on a debug build.
#endif

#ifdef _WWISE_LOGS
bool wwiseLogging = true; // You ARE on a Wwise logging build.
#else
bool wwiseLogging = false; // You are NOT on a Wwise logging build.
#endif

#ifndef _RSMODS_VERSION
#define _RSMODS_VERSION "RSMODS Version: 1.2.8.0 SRC. DEBUG: " << std::boolalpha << debug << ". Wwise Logs: " << std::boolalpha << wwiseLogging << "."
#endif

/// <summary>
/// Handle Force Enumeration
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
unsigned WINAPI EnumerationThread() {

	// User has not seen the main menu yet.
	// Please wait until the user has seen the main menu before doing anything.
	while (!GameState::GameLoaded)
		Sleep(5000);


	// Read the users Keybinds and Mod Settings, so verify we have the latest data from the INI.
	Settings::ReadKeyBinds();
	Settings::ReadModSettings();

	// Set an inital count for number of DLC songs.
	int oldDLCCount = Enumeration::GetCurrentDLCCount(), newDLCCount = oldDLCCount;

	// Look for new dlc files. Break when game is closing.
	while (!GameState::GameClosing) {
		if (Settings::ReturnSettingValue("ForceReEnumerationEnabled") == "automatic") {
			oldDLCCount = newDLCCount;
			newDLCCount = Enumeration::GetCurrentDLCCount();

			// If there is any new dlc files, trigger Enumeration.
			if (oldDLCCount != newDLCCount)
				Enumeration::ForceEnumeration();
		}

		// Sleep for a user provided interval (milliseconds).
		Sleep(Settings::GetModSetting("CheckForNewSongsInterval"));
	}

	return 0;
}

/// <summary>
/// Send Midi Data Async. Only really used in debug builds to test MIDI commands.
/// Secondary purpose of remaking D3D textures every 32 ticks (~ 1 second).
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
unsigned WINAPI MidiThread() {
	// Initial some values.
	int everyXcyclesCheckD3DTextures = 31;
	int currentCount = 0;

	while (!GameState::GameClosing) {
		// If this is the 32nd loop, remake the D3D textures.
		// This allows us to have real-time updates to textures.
		if (currentCount == 31) {
			currentCount = 0;
			D3DHooks::RecreateTextureTimer = true;
		}

		// If we have sent a Midi PC value to this thread, send the Midi value.
		if (Midi::sendPC)
			Midi::SendProgramChange(Midi::dataToSendPC);

		// If we have sent a Midi CC value to this thread, send the Midi value.
		if (Midi::sendCC)
			Midi::SendControlChange(Midi::dataToSendCC);

		// Sleep for ~ 1/33rd of a second so we don't waste that many cycles.
		Sleep(Midi::sleepFor); 
		currentCount++;
	}
	
	return 0;
}

unsigned WINAPI RiffRepeaterThread() {
	// Wait for the game to enter the main menu before attempting to read the current song info, in order to prevent crashes
	while (!GameState::GameLoaded)
		Sleep(5000);

	std::string previousSongKey = "";

	// We can only user Riff Repeater while the game is open, so verify it's open. Runs every 100 ms.
	while (!GameState::GameClosing) {
		Sleep(100);

		if (Settings::ReturnSettingValue("RRSpeedAboveOneHundred") != "on")
		{
			continue;
		}

		// Get SongKey for the song. We need this to reference the audio event by name.
		const auto songKey = GameState::GetSongKey();

		// If this isn't the song we saw on the last loop.
		if (songKey != previousSongKey) {
			previousSongKey = songKey;

			// If we have cached this event, then use the cached version, and tell the other threads to enable the Riff Repeater speed mod.
			if (RiffRepeater::SongObjectIDs.find("Play_" + previousSongKey) != RiffRepeater::SongObjectIDs.end()) {
				RiffRepeater::currentSongID = RiffRepeater::SongObjectIDs.find("Play_" + previousSongKey)->second;
				RiffRepeater::readyToLogSongID = false;
				RiffRepeater::loggedCurrentSongID = true;
			}
			// We have not seen this event yet, so we need to log the internal ID for the song and cache it.
			else {
				RiffRepeater::loggedCurrentSongID = false;
				RiffRepeater::readyToLogSongID = true; // Wait until the user gets into the song so we can grab this ID.
			}
		}

		// If we have recently changed the Riff Repeater speed through the mod, then save the new value to a text file.
		// This is primarily for streamers who want to make a custom overlay with the song speed.
		if (GameState::Menus::IsInModesWithAllowedFastRiffRepeater() && saveNewRRSpeedToFile) {
			auto rrText = std::ofstream("riff_repeater_speed.txt", std::ofstream::out);
			rrText << std::to_string((int)realSongSpeed) << std::endl;
			saveNewRRSpeedToFile = false;
		}
	}

	return 0;
}

/// <summary>
/// Handle Keypress Inputs. Used to toggle mods on / off. WARNING: RUNS (almost) EVERY FRAME
/// </summary>
/// <param name="hWnd"> - ID of Rocksmith</param>
/// <param name="msg"> - Reason Function was called. KEYUP = Keypress | COPYDATA = Settings Update | CLOSE = Game Closing.</param>
/// <param name="keyPressed"> - Virtual Key of the key pressed. Reference here: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes </param>
/// <param name="lParam"> - Data Sent</param>
/// <returns>Verification that message was sent.</returns>
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM keyPressed, LPARAM lParam) {
	_LOG_INIT;

	// Makes ALT + ENTER cause F11 to be pressed.
	// This is mainly so a user can use a common shortcut, that works in most games now-a-days.
	if (msg == WM_SYSCOMMAND && keyPressed == SC_KEYMENU) {
		if (lParam == VK_RETURN) 
			WndProc(hWnd, WM_KEYUP, VK_F11, 0);

		return true;
	}
	
	// Prevent a weird bug when trying to play Guitarcade or Score Attack with a key combination.
	if (msg == WM_SYSCOMMAND && keyPressed == SC_MOVE + 0x2 && GameState::Menus::IsInOnlineModes())
		return true;

	// Trigger Mod on Keypress
	if (msg == WM_KEYUP) {
		if (GameState::GameLoaded) { // Game must not be on the startup videos or it will crash

			// Toggle Loft mod
			if (keyPressed == Settings::GetKeyBind("ToggleLoftKey") && Settings::ReturnSettingValue("ToggleLoftEnabled") == "on") {
				Loft::ToggleLoft();
				_LOG("Triggered Mod: Toggle Loft" << std::endl);
			}

			// Show Song Timer mod
			else if (keyPressed == Settings::GetKeyBind("ShowSongTimerKey") && Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on") {
				D3DHooks::showSongTimerOnScreen = !D3DHooks::showSongTimerOnScreen;
				_LOG("Triggered Mod: Show Song Timer" << std::endl);
			}

			// Force Enumeration mod
			else if (keyPressed == Settings::GetKeyBind("ForceReEnumerationKey") && Settings::ReturnSettingValue("ForceReEnumerationEnabled") == "manual") {
				Enumeration::ForceEnumeration();
				_LOG("Triggered Mod: Force Enumeration" << std::endl);
			}

			// Rainbow Strings mod
			else if (keyPressed == Settings::GetKeyBind("RainbowStringsKey") && Settings::ReturnSettingValue("RainbowStringsEnabled") == "on") {
				ERMode::ToggleRainbowMode();

				if (!ERMode::RainbowEnabled)
					ERMode::ResetAllStrings();

				_LOG("Triggered Mod: Rainbow Strings" << std::endl);
			}

			// Rainbow Notes mod
			else if (keyPressed == Settings::GetKeyBind("RainbowNotesKey") && Settings::ReturnSettingValue("RainbowNotesEnabled") == "on") {
				ERMode::ToggleRainbowNotes();
				_LOG("Triggered Mod: Rainbow Notes" << std::endl);
			}

			// Remove Lyrics mod
			else if (keyPressed == Settings::GetKeyBind("RemoveLyricsKey") && Settings::ReturnSettingValue("RemoveLyricsWhen") == "manual") {
				D3DHooks::RemoveLyrics = !D3DHooks::RemoveLyrics;
				_LOG("Triggered Mod: Remove Lyrics" << std::endl);
			}

			// Control + A. Force us to read the Settings from the INI again, to renew our cached values.
			else if (keyPressed == 0x41 && (GetKeyState(VK_CONTROL) & 0x8000)) {
				Settings::UpdateSettings();
				_LOG("Triggered Setting Update" << std::endl);
			}

			// Changed Displayed Volume mod
			else if (keyPressed == Settings::GetKeyBind("ChangedSelectedVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				GameOverlay::currentVolumeIndex++;

				if (GameOverlay::currentVolumeIndex > (GameOverlay::mixerInternalNames.size() - 1)) // There are only so many values we know we can edit, so loop back.
					GameOverlay::currentVolumeIndex = 0;
			}

			// Change Tuning Offset mod
			else if (keyPressed == Settings::GetKeyBind("TuningOffsetKey") && Settings::ReturnSettingValue("AutoTuneForSong") == "on") {
				if (GetKeyState(VK_CONTROL) & 0x8000) // Is Control Pressed
					Midi::tuningOffset--;
				else
					Midi::tuningOffset++;

				// Cap what values can be set (as we don't want to have to specify every value, and some pedals don't support more than an octave).
				if (Midi::tuningOffset < -3)
					Midi::tuningOffset = -3;
				else if (Midi::tuningOffset > 12)
					Midi::tuningOffset = 12;

				_LOG("Triggered Mod Setting: Tuning Offset is now set to " << Midi::tuningOffset << std::endl);
			}

			// Toggle Extended Range mod
			else if (keyPressed == Settings::GetKeyBind("ToggleExtendedRangeKey"))
			{
				D3DHooks::UseERExclusivelyInThisSong = !D3DHooks::UseERExclusivelyInThisSong;
				GameState::ToggleCB(D3DHooks::UseERExclusivelyInThisSong);

				_LOG("Triggered Mod: Toggle Extended Range" << std::endl);
			}

			// Looping mod. Set loop starting point.
			else if (keyPressed == Settings::GetKeyBind("LoopStartKey") && Settings::ReturnSettingValue("AllowLooping") == "on" && GameState::Menus::IsInModesWithAllowedFastRiffRepeater()) {
				if (GetKeyState(VK_CONTROL) & 0x8000) { // Is Control Pressed.
					loopStart = NULL;
					loopEnd = NULL;
				}
				else {
					// Set the start of the loop to the current time in the song.
					loopStart = SongTimer::SongTimer();

					// If end point of the loop comes at the same time as, or before, the start of the loop, reset it to 0.
					if (loopEnd <= loopStart) {
						loopEnd = NULL;
					}
				}
			}

			// Looping mod. Set loop ending point.
			else if (keyPressed == Settings::GetKeyBind("LoopEndKey") && Settings::ReturnSettingValue("AllowLooping") == "on" && GameState::Menus::IsInModesWithAllowedFastRiffRepeater()) {
				if (GetKeyState(VK_CONTROL) & 0x8000) { // Is Control Pressed.
					loopEnd = NULL;
				}
				else {
					// Set the end of the loop to the current time in the song.
					loopEnd = SongTimer::SongTimer();

					// If end point of the loop comes at the same time as, or before, the start of the loop, reset it to 0.
					if (loopEnd <= loopStart) {
						loopEnd = NULL;
					}
				}
			}

			// Rewind song by X seconds mod.
			else if (keyPressed == Settings::GetKeyBind("RewindKey") && Settings::ReturnSettingValue("AllowRewind") == "on" && GameState::Menus::IsInLASPlayingModes()) {
				// SongTimer is stored in seconds, while RewindBy is stored in milliseconds.
				// We need milliseconds to send to Wwise, so change SongTimer to milliseconds, then subtract the Rewind value.
				AkTimeMs seekTo = (AkTimeMs)((SongTimer::SongTimer() * 1000) - Settings::GetModSetting("RewindBy"));

				// RewindBy is greater than the amount of time we've been in the song.
				// Reset seekTo to 0 to prevent seeking to a negative time.
				if (seekTo < 0) {
					_LOG("(REWIND) Tried to seek to " << seekTo << "ms into the song. Resetting to 0." << std::endl);
					seekTo = 0;
				}

				// Send event to Wwise to rewind the song.
				// Or more accurately, move to the seek time since Wwise doesn't have a rewind function.
				Wwise::SoundEngine::SeekOnEvent(std::string("Play_" + GameState::GetSongKey()).c_str(), 0x1234, seekTo, false);

				// Tell Rocksmith to make all notes before the section we want the user to play to be greyed out.
				// While this isn't absolutely necessary, it is best to have this run just in case.
				// Our seek time needs to be stored as milliseconds when sending to Wwise, but we need to have it in seconds when setting the GreyNoteTimer.
				SongTimer::SetGreyNoteTimer(seekTo / 1000.f);

				_LOG("(REWIND) Seeked to " << seekTo << "ms." << std::endl);
			}

			else if (keyPressed == Settings::GetKeyBind("MutePlayer1Key"))
			{
				if (VolumeControl::player1Muted)
				{
					VolumeControl::UnmutePlayer();
				}
				else
				{
					VolumeControl::MutePlayer();
				}

				GameOverlay::displayCurrentVolume = true;
				GameOverlay::displayVolumeStartTime = std::chrono::steady_clock::now();
				GameOverlay::currentVolumeIndex = 2;
			}

			else if (keyPressed == Settings::GetKeyBind("MutePlayer2Key"))
			{
				if (VolumeControl::player2Muted)
				{
					VolumeControl::UnmutePlayer(true);
				}
				else
				{
					VolumeControl::MutePlayer(true);
				}

				GameOverlay::displayCurrentVolume = true;
				GameOverlay::displayVolumeStartTime = std::chrono::steady_clock::now();
				GameOverlay::currentVolumeIndex = 3;
			}

			// Hide the mixer if it is not actively being pressed
			else if (keyPressed == Settings::GetKeyBind("DisplayMixerKey")) {
				GameOverlay::displayMixer = false;
			}

			// Auto Tuning via MIDI mod. 
			// Checks if we are in a tuning menu, and the user tried to skip tuning.
			if (Settings::ReturnSettingValue("AutoTuneForSongWhen") == "manual" && GameState::Menus::IsInTuningMenus() && keyPressed == VK_DELETE) {
				Midi::userWantsToUseAutoTuning = true;
			}
		}

		// If we are using a debug version of the mods.
		if (debug) {
			if (keyPressed == VK_INSERT) { // Debug menu. ImGUI
				D3DHooks::menuEnabled = !D3DHooks::menuEnabled;
			}
		}
	}

	// Repeatedly trigger mod on key hold
	else if (msg == WM_KEYDOWN) {
		// Game must not be on the startup videos or it might crash
		if (GameState::GameLoaded) {
			// Riff Repeater > 100% mod.
			if (keyPressed == Settings::GetKeyBind("RRSpeedKey") && Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && GameState::Menus::IsInModesWithAllowedFastRiffRepeater() && RiffRepeater::loggedCurrentSongID) {

				// Get the current speed of Riff Repeater.
				realSongSpeed = RiffRepeater::GetSpeed(true);

				// Add / Subtract User specified Interval
				if (GetKeyState(VK_CONTROL) & 0x8000)
					realSongSpeed -= (float)Settings::GetModSetting("RRSpeedInterval");
				else
					realSongSpeed += (float)Settings::GetModSetting("RRSpeedInterval");

				// Set limits to the speed
				// Cap at 400. Plugin only goes down to 25. 10000 / 25 = 400.
				// Cap at 25. Plugin only goes up to 400. 10000 / 400 = 25.

				if (realSongSpeed > 400.f)
					realSongSpeed = 400.f;

				if (realSongSpeed < 25.f)
					realSongSpeed = 25.f;

				// Save new speed, and save it to a file (for streamers to use as a custom on-screen overlay)
				RiffRepeater::SetSpeed(realSongSpeed, true);
				RiffRepeater::EnableTimeStretch();
				saveNewRRSpeedToFile = true;

				_LOG("Triggered Mod: Song Speed set to " << realSongSpeed << "% which is equivalent to " << RiffRepeater::ConvertSpeed(realSongSpeed) << " Wwise RTPC." << std::endl);
			}

			// Display Mixer  mod
			else if (keyPressed == Settings::GetKeyBind("DisplayMixerKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				GameOverlay::displayMixer = true;
			}

			// Change Master Volume mod
			else if (keyPressed == Settings::GetKeyBind("MasterVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Master_Volume");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Master_Volume");

				displayCurrentVolume = true;
				displayVolumeStartTime = std::chrono::steady_clock::now();
				currentVolumeIndex = 0;
			}

			// Change Song Volume mod
			else if (keyPressed == Settings::GetKeyBind("SongVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Music");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Music");

				displayCurrentVolume = true;
				displayVolumeStartTime = std::chrono::steady_clock::now();
				currentVolumeIndex = 1;
			}

			// Change P1 Guitar & P1 Bass Volume mod
			else if (keyPressed == Settings::GetKeyBind("Player1VolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player1");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player1");

				displayCurrentVolume = true;
				displayVolumeStartTime = std::chrono::steady_clock::now();
				currentVolumeIndex = 2;
			}

			// Change P2 Guitar & P2 Bass Volume mod
			else if (keyPressed == Settings::GetKeyBind("Player2VolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player2");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player2");

				displayCurrentVolume = true;
				displayVolumeStartTime = std::chrono::steady_clock::now();
				currentVolumeIndex = 3;
			}

			// Change Microphone Volume mod
			else if (keyPressed == Settings::GetKeyBind("MicrophoneVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Mic");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Mic");

				displayCurrentVolume = true;
				displayVolumeStartTime = std::chrono::steady_clock::now();
				currentVolumeIndex = 4;
			}

			// Change Voice-Over Volume mod
			else if (keyPressed == Settings::GetKeyBind("VoiceOverVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_VO");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_VO");

				displayCurrentVolume = true;
				displayVolumeStartTime = std::chrono::steady_clock::now();
				currentVolumeIndex = 5;
			}

			// Change SFX Volume mod
			else if (keyPressed == Settings::GetKeyBind("SFXVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_SFX");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_SFX");

				displayCurrentVolume = true;
				displayVolumeStartTime = std::chrono::steady_clock::now();
				currentVolumeIndex = 6;
			}
		}
	}

	// Update settings from GUI.
	// Done on GUI setting save.
	else if (msg == WM_COPYDATA) {
		auto pcds = (COPYDATASTRUCT*)lParam;
		if (pcds->dwData == 1)
		{
			std::string currMsg = (char*)pcds->lpData;
			_LOG(currMsg << std::endl);

			// GUI sent a "update" message. We need to re-read the INI.
			if (Contains(currMsg, "update")) {
				if (Contains(currMsg, "all"))
					Settings::UpdateSettings();
				else
					Settings::ParseSettingUpdate(currMsg);
			}

			// GUI sent a "WwiseEvent" message. We need to trigger a Voice-Over so the user can test their Sound Pack.
			else if (Contains(currMsg, "WwiseEvent")) {
				auto msgParts = Settings::SplitByWhitespace(currMsg);

				if (msgParts.size() == 2)
					VoiceOverControl::PlayVoiceOver(msgParts[1]);
			}

			// **Deprecated** Twitch Integration. Use CrowdControl.
			else if (Contains(currMsg, "TurboSpeed"))
			{
				if (Contains(currMsg, "enable"))
					RiffRepeater::EnableTimeStretch();
				else
					RiffRepeater::DisableTimeStretch();
			}
			else if (Contains(currMsg, "enable"))
				effectQueue.push_back(currMsg);
			else if (Contains(currMsg, "Reconnect"))
				CrowdControl::StartServerLoop();
		}
	}

	// Failsafe for mouse input falling through the menu
	//if (D3DHooks::menuEnabled && (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP || msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP || msg == WM_MOUSEWHEEL || msg == WM_MOUSEMOVE))
	//	return false;

	// If Game is closing, else get ImGUI setup.
	if (msg == WM_CLOSE)
		GameState::GameClosing = true;
	else {
		ImGuiIO& io = ImGui::GetIO();

		POINT mPos;
		GetCursorPos(&mPos);
		ScreenToClient(hWnd, &mPos);
		ImGui::GetIO().MousePos.x = mPos.x;
		ImGui::GetIO().MousePos.y = mPos.y;

		if (D3DHooks::menuEnabled && ImGui_ImplWin32_WndProcHandler(hWnd, msg, keyPressed, lParam))
			return true;
	}

	return CallWindowProc(D3DHooks::oWndProc, hWnd, msg, keyPressed, lParam);
}

/// <summary>
/// Hook on game boot. Initialize all our own UI elements (text on screen, and ImGUI).
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <returns>HRESULT of the official EndScene</returns>
HRESULT APIENTRY D3DHooks::Hook_EndScene(IDirect3DDevice9* pDevice) {
	_LOG_INIT;

	HRESULT originalReturn = oEndScene(pDevice);
	if (Menu::IsOverlayCall()) {
		return originalReturn;
	}

	Menu::Init(pDevice, (LONG_PTR)WndProc);
	Menu::RenderImGuiMenu();
	Menu::UpdateStringTextures(pDevice);
	GameOverlay::RenderOverlay(pDevice);
	D3DHooks::RegenerateTwitchNoteColors(pDevice);
	
	return originalReturn;
}

/// <summary>
/// Manage Queue of Twitch Effects.
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
unsigned WINAPI HandleEffectQueueThread() {
	while (!GameState::GameClosing) {
		if (Twitch::effectQueue.empty() && GameState::IsInSong()) {
			Twitch::ParseEffectQueue();
		}

		Sleep(250);
	}
	return 0;
}

/// <summary>
/// Hook Game Functions For Our Own Uses (On Alt-Tab, Draw UI, etc).
/// </summary>
void GUI() {
	_LOG_INIT;

	uint32_t d3d9Base;
	uint32_t adr;
	uint32_t* vTable = NULL;

	// Find D3D Device
	while ((d3d9Base = (uint32_t)GetModuleHandleA("d3d9.dll")) == NULL)
		Sleep(500);
	adr = MemUtil::FindPattern<uint32_t>(d3d9Base, Offsets::d3dDevice_SearchLen, (PBYTE)Offsets::d3dDevice_Pattern, Offsets::d3dDevice_Mask) + 2;

	bool runningThroughWine = adr == (uint32_t)2;

	_LOG("Running in Wine: " << std::boolalpha << runningThroughWine << std::endl);

	// Proton / Wine Check.
	// We do NOT support linux. There is some issues with the D3D pointers.
	// This check is meant so if someone does load our mods on Linux, we won't just crash. Most mods will just not work.
	if (runningThroughWine) {
		_LOG("Performing Wine check" << std::endl);
		adr = MemUtil::FindPattern<uint32_t>(0x2000000, 0x2B778CC, (PBYTE)Offsets::d3dDevice_Pattern, Offsets::d3dDevice_Mask) + 2;
		_LOG(adr << std::endl);
	}

	_LOG_SETLEVEL(LogLevel::Error);

	if (!adr) {
		_LOG("Could not find D3D9 device pointer." << std::endl);
		return;
	}

	if (!*(uint32_t*)adr) { // Wing it
		_LOG("Could not find DX9 device." << std::endl);
		MessageBoxA(NULL, "Could not find DX9 device, please restart the game!", "Error", NULL);
		return;
	}

	vTable = *(uint32_t**)adr;

	// Third time's the charm?
	if (!vTable || vTable < (uint32_t*)Offsets::baseHandle) {
		_LOG("Could not find D3D device's vTable address." << std::endl);
		MessageBoxA(NULL, "Could not find D3D device's vTable address \n Restart the game and if you still get this error after a few tries, please report the error!", "Error", NULL);
		return;
	}

	// Hook D3D functions to use for our own D3D work. Reference D3DHooks
	if (!runningThroughWine) {
		oSetVertexDeclaration = (tSetVertexDeclaration)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetVertexDeclaration_Index], (PBYTE)D3DHooks::Hook_SetVertexDeclaration, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexdeclaration
		oSetVertexShaderConstantF = (tSetVertexShaderConstantF)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetVertexShaderConstantF_Index], (PBYTE)D3DHooks::Hook_SetVertexShaderConstantF, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexshaderconstantf
		oReset = (tReset)DetourFunction((PBYTE)vTable[D3DInfo::Reset_Index], (PBYTE)D3DHooks::Hook_Reset); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-reset
	}

	oSetVertexShader = (tSetVertexShader)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetVertexShader_Index], (PBYTE)D3DHooks::Hook_SetVertexShader, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexshader
	oSetPixelShader = (tSetPixelShader)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetPixelShader_Index], (PBYTE)D3DHooks::Hook_SetPixelShader, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setpixelshader
	oSetStreamSource = (tSetStreamSource)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetStreamSource_Index], (PBYTE)D3DHooks::Hook_SetStreamSource, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setstreamsource
	oEndScene = (tEndScene)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::EndScene_Index], (PBYTE)D3DHooks::Hook_EndScene, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-endscene
	oDrawIndexedPrimitive = (tDrawIndexedPrimitive)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::DrawIndexedPrimitive_Index], (PBYTE)D3DHooks::Hook_DIP, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawindexedprimitive
	oDrawPrimitive = (tDrawPrimitive)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::DrawPrimitive_Index], (PBYTE)D3DHooks::Hook_DP, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawprimitive
}

/// <summary>
/// Update settings so users don't need to restart the game for every mod they want to toggle on / off.
/// </summary>
void UpdateSettings() {
	Settings::UpdateSettings();
	Sleep(500);
	CustomSongTitles::LoadSettings();
	Sleep(500);
	CustomSongTitles::HookSongListsKoko();
	Sleep(500);
}

/// <summary>
/// Main Thread where we trigger the mods to startup.
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
unsigned WINAPI MainThread() {
	_LOG_INIT;

	if (!(std::ifstream("RSMods.ini"))) {
		std::ofstream RSModsFileOutput("RSMods.ini"); // If we don't call this, the game will crash.
		RSModsFileOutput.close();
	}

	_LOG_NOHEAD(_RSMODS_VERSION << std::endl); // Put version info in the output log.

	bool movedToExternalDisplay = false; // User wants to move the display to a specific location on boot.
	bool skipERSleep = false; // If using RR past 100%, remove the 1.5s sleep on ER mode, to stop flickering colors.
	bool forkInToasterNewProfile = false; // If Auto Load Profile has a specified profile, and we can't find it, then this will be true.

	// Initialize Functions
	D3DHooks::debug = debug;
	Offsets::Initialize();

	BugPrevention::FixModifyingFunctions();
	Settings::Initialize();

	UpdateSettings();
	ERMode::Initialize();

	GUI();
	Midi::InitMidi();
	Enumeration::HookEnumerationService();

	// Misc mods / Bug Prevention that can only be ran on startup.
	Midi::tuningOffset = Settings::GetModSetting("TuningOffset");
	AudioDevices::SetupMicrophones();
	BugPrevention::PreventPnPCrash();
	QualityOfLife::StopTwoRSInstances();

	BugPrevention::AllowComplexPasswords();
	BugPrevention::PreventAdvancedDisplayCrash();
	BugPrevention::PreventPortAudioInDeviceCrash();
	BugPrevention::PreventExtraAudioDevicesCrash();

	if (Settings::ReturnSettingValue("FixBrokenTones") == "on")
		BugPrevention::PreventStuckTone();

	if (Settings::ReturnSettingValue("FixOculusCrash") == "on")
		BugPrevention::PreventOculusCrash();

	// AltOutputSampleRate mod.
	// We have to do this early in execution as we need to change it before the audio engine starts up.
	if (Settings::ReturnSettingValue("AltOutputSampleRate") == "on" && Settings::GetModSetting("AlternativeOutputSampleRate") != 48000) {
		_LOG("[!] Overriding Output Sample Rate to " << Settings::GetModSetting("AlternativeOutputSampleRate") << std::endl);
		AudioDevices::output_SampleRate = Settings::GetModSetting("AlternativeOutputSampleRate");
		AudioDevices::ChangeOutputSampleRate();
	}

	// Look to see if RS_ASIO applied the 2 RTC input bypass.
	// If they did, then we disregard the results from our version of the mod.

	bool rs_asio_BypassTwoRTC = false;
	_LOG("RS_ASIO Bypass2RTC: " << std::boolalpha << rs_asio_BypassTwoRTC << std::endl);
	if (Settings::ReturnSettingValue("BypassTwoRTCMessageBox") == "on")
		QualityOfLife::PatchTwoRTC();

	// Patch x86 assembly for Riff Repeater speed logic to make it linear.
	if (Settings::ReturnSettingValue("LinearRiffRepeater") == "on")
		RiffRepeater::EnableLinearSpeeds();

#ifdef _WWISE_LOGS // Only use in a debug environment. Will fill your log with spam!
	Wwise::Logging::Init();
#endif

	// Allow the user to have a small amount of time to Alt+Tab while the game continues playing the audio.
	if (Settings::ReturnSettingValue("AllowAudioInBackground") == "on")
		VolumeControl::AllowAltTabbingWithAudio();

	using namespace D3DHooks;
	while (!GameState::GameClosing) {
		Sleep(250); // We don't need to call these settings always, we just want it to run every 1/4 of a second so the user doesn't notice it.

		// Move Rocksmith to second monitor on boot (if specified)
		if (!movedToExternalDisplay && Settings::ReturnSettingValue("SecondaryMonitor") == "on") {
			LaunchOnExternalMonitor::SendRocksmithToScreen(Settings::GetModSetting("SecondaryMonitorXPosition"), Settings::GetModSetting("SecondaryMonitorYPosition")); // Move to Location in INI
			movedToExternalDisplay = true;
		}

		// If Game Is Loaded (No need to run these while the game is loading.)
		if (GameState::GameLoaded) {
			GameState::currentMenu = GameState::GetCurrentMenu(); // This loads without checking if memory is safe... This can cause crashes if used when GameLoaded is false.

			// Override the default microphone volume.
			if (Settings::ReturnSettingValue("OverrideInputVolumeEnabled") == "on" && Settings::ReturnSettingValue("OverrideInputVolumeDevice") != "" && AudioDevices::GetMicrophoneVolume(Settings::ReturnSettingValue("OverrideInputVolumeDevice")) != Settings::GetModSetting("OverrideInputVolume"))
				AudioDevices::SetMicrophoneVolume(Settings::ReturnSettingValue("OverrideInputVolumeDevice"), Settings::GetModSetting("OverrideInputVolume"));

			// User originally wanted to NOT allow audio in the background, but they changed their mind, so we have to turn it on/
			if (Settings::ReturnSettingValue("AllowAudioInBackground") == "on" && !VolumeControl::allowedAltTabbingWithAudio)
				VolumeControl::AllowAltTabbingWithAudio();

			// User originally wanted to allow audio in the background, but they changed their mind, so we have to turn it off.
			else if (Settings::ReturnSettingValue("AllowAudioInBackground") == "off" && VolumeControl::allowedAltTabbingWithAudio) {
				VolumeControl::DisableAltTabbingWithAudio();
			}

			// If the bypass for the 2 RTC dialog was set by RS_ASIO, don't change it!
			if (!rs_asio_BypassTwoRTC) {

				// User originally had BypassTwoRTCMessageBox on, but now they want it turned off.
				if (Settings::ReturnSettingValue("BypassTwoRTCMessageBox") == "off" && *(char*)Offsets::ptr_twoRTCBypass.Get() == Offsets::ptr_twoRTCBypass_patch_call[0])
					MemUtil::PatchAdr((LPVOID)Offsets::ptr_twoRTCBypass.Get(), (LPVOID)Offsets::ptr_twoRTCBypass_original, 6);

				// User originally had BypassTwoRTCMessageBox off, but now they want it turned on
				else if (Settings::ReturnSettingValue("BypassTwoRTCMessageBox") == "on" && *(char*)Offsets::ptr_twoRTCBypass.Get() == Offsets::ptr_twoRTCBypass_original[0])
					QualityOfLife::PatchTwoRTC();
			}

			// User wants NSP timer changed, and the time limit is not what the user set.
			if (Settings::ReturnSettingValue("UseCustomNSPTimer") == "on" && SongTimer::GetNonStopPlayTimer() != (Settings::GetModSetting("CustomNSPTimeLimit") / 1000.0))
				SongTimer::SetNonStopPlayTimer(Settings::GetModSetting("CustomNSPTimeLimit") / 1000.0);

			// The user originally wanted NSP timer changed, but now they disabled the mod.
			if (Settings::ReturnSettingValue("UseCustomNSPTimer") == "off" && SongTimer::GetNonStopPlayTimer() != DefaultNSPTimeLimit)
				SongTimer::SetNonStopPlayTimer(DefaultNSPTimeLimit);

			// User had Linear RR off, but now they want it turned on.
			if (Settings::ReturnSettingValue("LinearRiffRepeater") == "on" && !RiffRepeater::currentlyEnabled_LinearRR)
				RiffRepeater::EnableLinearSpeeds();

			// User had Linear RR on, but now they want it turned off.
			else if (Settings::ReturnSettingValue("LinearRiffRepeater") == "off" && RiffRepeater::currentlyEnabled_LinearRR)
				RiffRepeater::DisableLinearSpeeds();

			// Scan for MIDI devices for Automated Tuning / True-Tuning
			if (!Midi::scannedForMidiDevices && Settings::ReturnSettingValue("AutoTuneForSong") == "on") {
				Midi::scannedForMidiDevices = true;
				Midi::ReadMidiSettingsFromINI(Settings::ReturnSettingValue("ChordsMode"), Settings::GetModSetting("TuningPedal"), Settings::ReturnSettingValue("AutoTuneForSongDevice"), Settings::ReturnSettingValue("MidiInDevice"));
			}

			// Scan for MIDI In devices.
			if (!Midi::attemptedToDetachMidiInThread && Settings::ReturnSettingValue("MidiInDevice") != "") {
				Midi::attemptedToDetachMidiInThread = true;
				Midi::FindMidiInDevices(Settings::ReturnSettingValue("MidiInDevice")); // Just in-case the user has AutoTuneForSong off but MidiInDevice selected.
				std::thread(Midi::ListenToMidiInThread).detach();
			}

			// If User Is Entering / In Lesson Mode

			GameState::LessonMode = GameState::Menus::IsInLessonModes();

			/// Always on Mods (If the user specifies them to be on)

			// Remove Headstock (Always Off)
			if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && Settings::ReturnSettingValue("RemoveHeadstockWhen") == "startup")
				RemoveHeadstockInThisMenu = true; // In this case, the user always wants to remove the headstock. This value should never turn to false in this mode.

			// Toggle Loft (In Song / Always Off). Turn off in Lesson Mode (or the videos won't appear). Emulate effect with GreenScreenWall.
			if (GameState::LessonMode && Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings::ReturnSettingValue("ToggleLoftWhen") != "manual") {
				if (LoftOff)
					Loft::ToggleLoft();
				LoftOff = false;
				GreenScreenWall = true;
			}

			// Show Selected Volume
			if (Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {

				// Stop displaying volume if 3 seconds have passed since last adjustment
				const auto currentTime = std::chrono::steady_clock::now();
				if (currentTime - displayVolumeStartTime > std::chrono::seconds(3))
					displayCurrentVolume = false;
			}

			// Toggle Loft off (Always Off)
			if (!LoftOff && !GameState::LessonMode && Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings::ReturnSettingValue("ToggleLoftWhen") == "startup") {
				Loft::ToggleLoft();
				LoftOff = true;
				GreenScreenWall = false;
			}

			// Toggle Skyline (Always Off)
			if (!SkylineOff && Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings::ReturnSettingValue("ToggleSkylineWhen") == "startup")
				toggleSkyline = true;

			// Remove Lyrics (Always Off)
			if (!RemoveLyrics && Settings::ReturnSettingValue("RemoveLyricsWhen") == "startup")
				RemoveLyrics = true;

			// MIDI Auto Tuning / Auto True-Tuning (In Tuner)
			if (GameState::Menus::IsInPreSongTuner() && Settings::ReturnSettingValue("AutoTuneForSong") == "on" && Settings::ReturnSettingValue("AutoTuneForSongWhen") == "tuner" && !Midi::alreadyAttemptedTuningInTuner && !Midi::alreadyAutomatedTuningInThisSong) {
				Midi::AttemptTuningInTuner();
				skipERSleep = true;
			}
			/// If User Is Entering Song
			if (GameState::IsInSong()) {
				GuitarSpeakPresent = false;
				AttemptedERInTuner = false;
				UseERInTuner = false;

				// We are in a song we've haven't seen in this play session. Log its Id so we can prep for the Riff Repeater > 100% mod.
				if (RiffRepeater::readyToLogSongID) {
					if (RiffRepeater::LogSongID(GameState::GetSongKey()))
						RiffRepeater::readyToLogSongID = false;
				}

				// Enable riff repeater time stretching
				if (Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on")
					RiffRepeater::EnableTimeStretch();

				// Remove Headstock (In Song)
				if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && Settings::ReturnSettingValue("RemoveHeadstockWhen") == "song")
					RemoveHeadstockInThisMenu = true;

				// Toggle Loft (In Song)
				if (Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings::ReturnSettingValue("ToggleLoftWhen") == "song") {
					if (!LoftOff)
						Loft::ToggleLoft();
					LoftOff = true;
				}

				// Remove Skyline (In Song)
				if (Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings::ReturnSettingValue("ToggleSkylineWhen") == "song") {
					if (!SkylineOff)
						toggleSkyline = true;
					DrawSkylineInMenu = false;
				}

				// MIDI Auto Tuning / Auto True-Tuning (In Song)
				if (Settings::ReturnSettingValue("AutoTuneForSong") == "on" && !Midi::alreadyAutomatedTuningInThisSong && (Settings::ReturnSettingValue("AutoTuneForSongWhen") == "tuner" || (Settings::ReturnSettingValue("AutoTuneForSongWhen") == "manual" && Midi::userWantsToUseAutoTuning)))
					Midi::AutomateTuning();

				// Show Song Timer (In Song)
				if (!AutomatedSongTimer && Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on" && Settings::ReturnSettingValue("ShowSongTimerWhen") == "automatic") {
					AutomatedSongTimer = true;
					showSongTimerOnScreen = true;
				}

				// Attempt to turn on Extended Range
				if (!AttemptedERInThisSong) {
					if (!skipERSleep)
						Sleep(1500); // Tuning takes a second, or so, to get set by the game. We use this to make sure we have the right tuning numbers. Otherwise, we would never get ER mode to turn on properly.
					UseERExclusivelyInThisSong = SongTuning::IsExtendedRangeSong();
					UseEROrColorsInThisSong = (Settings::ReturnSettingValue("ExtendedRangeEnabled") == "on" && UseERExclusivelyInThisSong || Settings::GetModSetting("CustomStringColors") == 2 || (Settings::ReturnSettingValue("SeparateNoteColors") == "on" && Settings::GetModSetting("SeparateNoteColorsMode") != 1));
					AttemptedERInThisSong = true;
				}
			}

			/// If User Is Exiting A Song / In A Menu

			else {

				/// Turn on Extended Range In Tuner
				if (GameState::Menus::IsInPreSongTuner()) {
					if (!AttemptedERInTuner) { // The reason this is a separate if statement is so that the else statement isn't voiding the correct menu.
						if (!skipERSleep)
							Sleep(1500); // Tuning takes a second, or so, to get set by the game. We use this to make sure we have the right tuning numbers. Otherwise, we would never get ER mode to turn on properly.
						AttemptedERInTuner = true;
						UseERInTuner = SongTuning::IsExtendedRangeTuner();
					}
				}
				else {
					AttemptedERInTuner = false;
					UseERInTuner = false;
				}

				// Turn off Looping mod
				if (Settings::ReturnSettingValue("AllowLooping") == "on") {
					if (loopStart != NULL)
						loopStart = NULL;

					if (loopEnd != NULL)
						loopEnd = NULL;
				}

				// Turn off Riff Repeater Speed above 100%
				if (!GameState::Menus::IsInScoreMenus() && RiffRepeater::currentlyEnabled_Above100) {
					RiffRepeater::DisableTimeStretch();
				}

				// Turn off Extended Range
				if (AttemptedERInThisSong) {
					UseERExclusivelyInThisSong = false;
					UseEROrColorsInThisSong = false;
					AttemptedERInThisSong = false;
				}

				// Turn off Show Song Timer (In Song)
				if (AutomatedSongTimer && Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on" && Settings::ReturnSettingValue("ShowSongTimerWhen") == "automatic") {
					AutomatedSongTimer = false;
					showSongTimerOnScreen = false;
				}

				// Turn off MIDI Auto Tuning / Auto True-Tuning
				if ((Midi::alreadyAutomatedTuningInThisSong || Midi::alreadyAttemptedTuningInTuner) && !GameState::Menus::IsInPreSongTuner()) {
					Midi::RevertAutomatedTuning();
					Midi::alreadyAttemptedTuningInTuner = false;
					Midi::userWantsToUseAutoTuning = false;
				}

				// Turn off Remove Headstock (In Song)
				if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && Settings::ReturnSettingValue("RemoveHeadstockWhen") == "song")
					RemoveHeadstockInThisMenu = false;

				// Turn loft back on. If leaving lesson mode, turn off GreenScreenWall.
				if (Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings::ReturnSettingValue("ToggleLoftWhen") == "song") {
					if (LoftOff) {
						Loft::ToggleLoft();
						LoftOff = false;
					}
					if (!GameState::LessonMode)
						GreenScreenWall = false;
				}

				// Turn off Remove Skyline (In Song)
				if (SkylineOff && Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings::ReturnSettingValue("ToggleSkylineWhen") == "song") {
					toggleSkyline = true;
					DrawSkylineInMenu = true;
				}

				// Turn on Guitar Speak
				if (!GuitarSpeakPresent && Settings::ReturnSettingValue("GuitarSpeak") == "on") { // Guitar Speak
					GuitarSpeakPresent = true;
					if (!GuitarSpeak::RunGuitarSpeak()) // If we are in a menu where we don't want to read bad values
						GuitarSpeakPresent = false;
				}

				// Disable song previews
				if (Settings::ReturnSettingValue("SongPreviews") == "on") {
					if (!VolumeControl::disabledSongPreviewAudio)
						VolumeControl::DisableSongPreviewAudio();
				}

				// User originally wanted song previews off, but now wants them on.
				else if (VolumeControl::disabledSongPreviewAudio)
					VolumeControl::EnableSongPreviewAudio();

				// Reset Headstock Cache (so we aren't running the same textures over and over again)
				if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && !GameState::Menus::IsInTuningMenus() || GameState::currentMenu == "MissionMenu")
					resetHeadstockCache = true;
			}

			/// "Other" menus. These will normally state what menus they need to be in.

			// Screenshot Scores
			if (Settings::ReturnSettingValue("ScreenShotScores") == "on" && GameState::Menus::IsInScoreMenus())
				Keyboard::TakeScreenshot();
			else
				Keyboard::takenScreenshotOfThisScreen = false;

			// If the current menu is not the same as the previous menu and if it's one of menus where you tune your guitar (i.e. headstock is shown), reset the cache because user may want to change the headstock style
			if (GameState::previousMenu != GameState::currentMenu && GameState::Menus::IsInTuningMenus()) {
				resetHeadstockCache = true;
				headstockTexturePointers.clear();
			}

			GameState::previousMenu = GameState::currentMenu;

			// Toggle Rainbow Strings / Rainbow Notes effect(s) if enabled.
			if (ERMode::IsRainbowEnabled() || ERMode::IsRainbowNotesEnabled())
				ERMode::DoRainbow();
			else
				ERMode::Toggle7StringMode();
		}

		/// Game Hasn't Loaded Yet

		else {

			// Change Current Menu status to the current menu while the game is loading.
			// This is the safe version of checking the current menu.
			// It is only used while the game boots, else the game may crash.
			GameState::currentMenu = GameState::GetCurrentMenu(true);

			// Have We Loaded? Or has the user opened a new user profile?
			// This prevents the user from being locked in a loop.
			if (GameState::currentMenu.compare("MainMenu") == 0 || GameState::currentMenu.compare("PlayedRS1Select") == 0 || GameState::currentMenu.compare("SimpleDialog") == 0)
				GameState::GameLoaded = true;

			// Set buffer settings if the user uses an alternative sample rate on their audio output.
			if (Settings::ReturnSettingValue("AltOutputSampleRate") == "on" && Settings::GetModSetting("AlternativeOutputSampleRate") != 48000 && *(int*)Offsets::ptr_sampleRateBuffer.Get() != 5 && *(int*)Offsets::ptr_sampleRateBuffer.Get() != 2) {
				*(int*)Offsets::ptr_sampleRateSize.Get() = 2;
				*(int*)Offsets::ptr_sampleRateBuffer.Get() = 128;
			}

			// Auto Load Profile. AKA "Fork in the toaster".
			if (Settings::ReturnSettingValue("ForceProfileEnabled") == "on" && !GameState::Menus::IsInMenusWithDisallowedAutoEnter() && !forkInToasterNewProfile) {
				// Skip UPlay login dialog - depending on the menu it might be necessary to press either ESC or Enter, so just spam both
				if (GameState::currentMenu == (std::string)"SelectionListDialog" || GameState::currentMenu == (std::string)"UplayLoginDialog") {
					Keyboard::SendEscapeKey();
					Keyboard::AutoEnterGame();
				}
				// If the user user says "I want to always load this profile"
				else if (Settings::ReturnSettingValue("ProfileToLoad") != "" && GameState::currentMenu == (std::string)"ProfileSelect") {
					selectedUser = GameState::CurrentSelectedUser();
					if (selectedUser == Settings::ReturnSettingValue("ProfileToLoad")) // The profile we're looking for
						Keyboard::AutoEnterGame();
					else if (selectedUser == (std::string)"New profile") { // Yeah, the profile they're looking for doesn't exist :(
						_LOG_SETLEVEL(LogLevel::Error);
						_LOG("(Auto Load) Invalid Profile Name" << std::endl);
						_LOG_SETLEVEL(LogSettings::defaultLogLevel);
						forkInToasterNewProfile = true;
					}
					else { // Not the profile we're looking for. Move down 1.
						Keyboard::PressDownArrowKey();
					}
				}
				// User doesn't care what profile we select, just select the first / top one.
				else
					Keyboard::AutoEnterGame();
			}
		}
	}
	return 0;
}

/// <summary>
/// Unhook all threads to take advantage of multi-threading.
/// </summary>
void Initialize() {
	LogSettings::startupTime = clock();

	Wwise::Exports::Initialize();

	std::thread(MainThread).detach(); // Mod Toggle based on menus
	/*std::thread(EnumerationThread).detach(); // Force Enumeration
	std::thread(HandleEffectQueueThread).detach(); // Twitch Effects
	std::thread(MidiThread).detach(); // MIDI Auto Tuning / True Tuning
	std::thread(RiffRepeaterThread).detach(); // RR Speed Above 100% Log

	// Probably check ini setting before starting this thing
	CrowdControl::StartServer(); // Twitch Effects Server
	*/
}

/// <summary>
/// Hook into the game for us to run our own code. **DISPLAYS DEBUG CONSOLE ON DEBUG BUILD**
/// </summary>
/// <param name="hModule"></param>
/// <param name="dwReason"></param>
/// <param name="lpReserved"></param>
/// <returns>Always returns TRUE</returns>
BOOL APIENTRY DllMain(HMODULE hModule, uint32_t dwReason, LPVOID lpReserved) {
	// Init boos
	bool debugLogPresent = std::ifstream("RSMods_debug.txt").good();
	auto clearDebugLog = std::ofstream("RSMods_debug.txt");

	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		// Setup logging system
		FILE* streamRead;
		FILE* streamConsole;

		if (debug) {
			AllocConsole();

			// Connect stdin, stdout to the debug console.
			freopen_s(&streamRead, "CONIN$", "r", stdin);
			freopen_s(&streamConsole, "CONOUT$", "w", stdout);
		}

		// Create log file to both help with debugging release builds,
		// and allow the user to examine their debug logs after a crash.
		if (debugLogPresent) {
			// Clear log so it isn't full of junk from the last launch
			clearDebugLog.open("RSMods_debug.txt", std::ofstream::out | std::ofstream::trunc);
			clearDebugLog.close();

			FILE* debugLog;
			freopen_s(&debugLog, "RSMods_debug.txt", "w", stderr);
		}

		DisableThreadLibraryCalls(hModule); // Disables the DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications. | https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-disablethreadlibrarycalls
		Proxy::Init(); // Proxy all real XInput commands to the actual xinput1_3.dll.
		Initialize(); // Inject our mod code.
		return TRUE;
	case DLL_PROCESS_DETACH:
		Proxy::Shutdown(); // Kill Proxy to xinput1_3.dll

		if (Menu::ImGuiInit)
		{
			ImGui_ImplWin32_Shutdown();
			ImGui_ImplDX9_Shutdown();
			ImGui::DestroyContext();
		}
		return TRUE;
	}
	return TRUE;
}
