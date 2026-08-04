#include "stdafx.h"
#include "ModManager.hpp"

using Settings::When;

namespace ModManager {
	void InitializeConfiguration() {
		if (!(std::ifstream("RSMods.ini"))) {
			std::ofstream RSModsFileOutput("RSMods.ini");
			RSModsFileOutput.close();
		}
	}

	/// <summary>
	/// Applies bug prevention patches for game crashing bugs and similar issues.
	/// </summary>
	void ApplyBugPrevention() {
		BugPrevention::PreventPnPCrash();
		QualityOfLife::StopTwoRSInstances();
		BugPrevention::AllowComplexPasswords();
		BugPrevention::PreventAdvancedDisplayCrash();
		BugPrevention::PreventPortAudioInDeviceCrash();
		BugPrevention::PreventExtraAudioDevicesCrash();

		if (Settings::IsOn("FixBrokenTones")) {
			BugPrevention::PreventStuckTone();
		}

		if (Settings::IsOn("FixOculusCrash")) {
			BugPrevention::PreventOculusCrash();
		}
	}

	/// <summary>
	/// Hook Game Functions For Our Own Uses (On Alt-Tab, Draw UI, etc).
	/// </summary>
	void GUI() {
		uint32_t d3d9Base;
		uint32_t adr;
		uint32_t* vTable = NULL;

		// Find D3D Device
		while ((d3d9Base = (uint32_t)GetModuleHandleA("d3d9.dll")) == NULL)
			Sleep(500);
		adr = MemUtil::FindPattern<uint32_t>(d3d9Base, Offsets::d3dDevice_SearchLen, (byte*)Offsets::d3dDevice_Pattern, Offsets::d3dDevice_Mask) + 2;

		bool runningThroughWine = adr == (uint32_t)2;

		LOG_INFO("Running in Wine: " << std::boolalpha << runningThroughWine << std::endl);

		// Proton / Wine Check.
		// We do NOT support linux. There is some issues with the D3D pointers.
		// This check is meant so if someone does load our mods on Linux, we won't just crash. Most mods will just not work.
		if (runningThroughWine) {
			LOG_INFO("Performing Wine check" << std::endl);
			adr = MemUtil::FindPattern<uint32_t>(0x2000000, 0x2B778CC, (byte*)Offsets::d3dDevice_Pattern, Offsets::d3dDevice_Mask) + 2;
			LOG_INFO(adr << std::endl);
		}

		if (!adr) {
			LOG_ERROR("Could not find D3D9 device pointer." << std::endl);
			return;
		}

		if (!*(uint32_t*)adr) { // Wing it
			LOG_ERROR("Could not find DX9 device." << std::endl);
			MessageBoxA(NULL, "Could not find DX9 device, please restart the game!", "Error", NULL);
			return;
		}

		vTable = *(uint32_t**)adr;

		// Third time's the charm?
		if (!vTable || vTable < (uint32_t*)Offsets::baseHandle) {
			LOG_ERROR("Could not find D3D device's vTable address." << std::endl);
			MessageBoxA(NULL, "Could not find D3D device's vTable address \n Restart the game and if you still get this error after a few tries, please report the error!", "Error", NULL);
			return;
		}

		// Hook D3D functions to use for our own D3D work. Reference D3DHooks
		if (!runningThroughWine) {
			oSetVertexDeclaration = (tSetVertexDeclaration)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetVertexDeclaration_Index], (byte*)D3DHooks::Hook_SetVertexDeclaration, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexdeclaration
			oSetVertexShaderConstantF = (tSetVertexShaderConstantF)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetVertexShaderConstantF_Index], (byte*)D3DHooks::Hook_SetVertexShaderConstantF, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexshaderconstantf
			oReset = (tReset)DetourFunction((byte*)vTable[D3DInfo::Reset_Index], (byte*)D3DHooks::Hook_Reset); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-reset
		}

		oSetVertexShader = (tSetVertexShader)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetVertexShader_Index], (byte*)D3DHooks::Hook_SetVertexShader, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexshader
		oSetPixelShader = (tSetPixelShader)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetPixelShader_Index], (byte*)D3DHooks::Hook_SetPixelShader, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setpixelshader
		oSetStreamSource = (tSetStreamSource)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetStreamSource_Index], (byte*)D3DHooks::Hook_SetStreamSource, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setstreamsource
		oEndScene = (tEndScene)MemUtil::TrampHook((byte*)vTable[D3DInfo::EndScene_Index], (byte*)D3DHooks::Hook_EndScene, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-endscene
		oDrawIndexedPrimitive = (tDrawIndexedPrimitive)MemUtil::TrampHook((byte*)vTable[D3DInfo::DrawIndexedPrimitive_Index], (byte*)D3DHooks::Hook_DIP, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawindexedprimitive
		oDrawPrimitive = (tDrawPrimitive)MemUtil::TrampHook((byte*)vTable[D3DInfo::DrawPrimitive_Index], (byte*)D3DHooks::Hook_DP, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawprimitive
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
	/// Initializes all core mods and hooks.
	/// </summary>
	void InitializeMods(bool debug) {
		D3DHooks::debug = debug;
		Offsets::Initialize();
		BugPrevention::FixModifyingFunctions();
		Settings::Initialize();
		UpdateSettings();
		ERMode::Initialize();
		GUI();
		Midi::InitMidi();
		Enumeration::HookEnumerationService();

		CrowdControl::StartServer();
	}

	// PatchTwoRTC overwrites 25 bytes of the connection check, so restoring it
	// needs the 25 bytes that were actually there, captured from the live
	// process before the first patch. The previous restore wrote 6 bytes from a
	// 3-byte string literal, stamping 2 out-of-bounds bytes into game code.
	static unsigned char twoRTCBypassOriginalBytes[25];
	static bool hasCapturedTwoRTCBypassOriginal = false;

	static void SetTwoRTCBypass(bool enable)
	{
		const bool isPatched =
			*(char*)Offsets::ptr_twoRTCBypass.Get() == Offsets::ptr_twoRTCBypass_patch_call[0];
		if (enable == isPatched) return;

		if (enable) {
			if (!hasCapturedTwoRTCBypassOriginal) {
				memcpy(
					twoRTCBypassOriginalBytes,
					(const void*)Offsets::ptr_twoRTCBypass.Get(),
					sizeof(twoRTCBypassOriginalBytes));
				hasCapturedTwoRTCBypassOriginal = true;
			}

			QualityOfLife::PatchTwoRTC();
		}
		else if (hasCapturedTwoRTCBypassOriginal) {
			MemUtil::PatchAdr(
				(LPVOID)Offsets::ptr_twoRTCBypass.Get(),
				twoRTCBypassOriginalBytes,
				sizeof(twoRTCBypassOriginalBytes));
		}
	}

	/// <summary>
	/// Applies all mods and fixes that must run at startup.
	/// </summary>
	void ApplyStartupMods()
	{
		AudioDevices::SetupMicrophones();
		ApplyBugPrevention();
		ApplyAudioDeviceConfiguration();

		#ifdef _WWISE_LOGS
				Wwise::Logging::Init();
		#endif

		// Look to see if RS_ASIO applied the 2 RTC input bypass.
		// If they did, then we disregard the results from our version of the mod.
		bool rsAsioBypassTwoRTC = false;
		LOG_INFO("RS_ASIO Bypass2RTC: " << std::boolalpha << rsAsioBypassTwoRTC << std::endl);

		if (Settings::IsOn("BypassTwoRTCMessageBox")) {
			SetTwoRTCBypass(true);
		}

		// Patch x86 assembly for Riff Repeater speed logic to make it linear.
		if (Settings::IsOn("LinearRiffRepeater")) {
			RiffRepeater::EnableLinearSpeeds();
		}

		// Allow the user to have a small amount of time to Alt+Tab while the game continues playing the audio.
		if (Settings::IsOn("AllowAudioInBackground")) {
			VolumeControl::AllowAltTabbingWithAudio();
		}
	}

	/// <summary>
	/// Configures audio device settings including sample rates.
	/// We have to do this early in execution as we need to change it before the audio engine starts up.
	/// </summary>
	void ApplyAudioDeviceConfiguration() 
	{
		Midi::tuningOffset = Settings::GetModSetting("TuningOffset");

		if (Settings::IsOn("AltOutputSampleRate") &&
			Settings::GetModSetting("AlternativeOutputSampleRate") != 48000) {
			LOG_WARNING("[!] Overriding Output Sample Rate to " << Settings::GetModSetting("AlternativeOutputSampleRate") << std::endl);
			AudioDevices::output_SampleRate = Settings::GetModSetting("AlternativeOutputSampleRate");
			AudioDevices::ChangeOutputSampleRate();
		}
	}


	/// <summary>
	/// Handles dynamic toggling of linear riff repeater mode.
	/// </summary>
	void HandleLinearRiffRepeaterToggle() {
		if (Settings::IsOn("LinearRiffRepeater") &&
			!RiffRepeater::currentlyEnabled_LinearRR) {
			RiffRepeater::EnableLinearSpeeds();
		}
		else if (Settings::IsOff("LinearRiffRepeater") &&
			RiffRepeater::currentlyEnabled_LinearRR) {
			RiffRepeater::DisableLinearSpeeds();
		}
	}

	/// <summary>
	/// Scans for MIDI devices when auto-tuning is enabled.
	/// </summary>
	void HandleMidiDeviceScanning() {
		if (!Midi::scannedForMidiDevices && Settings::IsOn("AutoTuneForSong")) {
			Midi::scannedForMidiDevices = true;
			Midi::ReadMidiSettingsFromINI(
				Settings::ReturnSettingValue("ChordsMode"),
				Settings::GetModSetting("TuningPedal"),
				Settings::ReturnSettingValue("AutoTuneForSongDevice"),
				Settings::ReturnSettingValue("MidiInDevice")
			);
		}

		if (!Midi::attemptedToDetachMidiInThread && Settings::ReturnSettingValue("MidiInDevice") != "") {
			Midi::attemptedToDetachMidiInThread = true;
			Midi::FindMidiInDevices(Settings::ReturnSettingValue("MidiInDevice"));
			std::thread(Midi::ListenToMidiInThread).detach();
		}
	}

	bool MoreThanThreeSecondsPassed() {
		const auto currentTime = std::chrono::steady_clock::now();
		return currentTime - GameOverlay::displayVolumeStartTime > std::chrono::seconds(3);
	}

	/// <summary>
	/// Manages the volume control overlay display timer.
	/// </summary>
	void HandleVolumeDisplay() {
		if (Settings::IsOn("VolumeControlEnabled") && MoreThanThreeSecondsPassed()) {
			GameOverlay::displayCurrentVolume = false;	
		}
	}

	/// <summary>
	/// Handles mods that run regardless of game state.
	/// </summary>
	void HandleAlwaysOnMods(GameLoopState& state) {
		HandleVolumeDisplay();

		if (GameState::Menus::IsInPreSongTuner() &&
			Settings::IsOn("AutoTuneForSong") &&
			Settings::GetWhen("AutoTuneForSongWhen") == When::Tuner &&
			!Midi::alreadyAttemptedTuningInTuner &&
			!Midi::alreadyAutomatedTuningInThisSong) {
			Midi::AttemptTuningInTuner();
			state.skipERSleep = true;
		}
	}

	/// <summary>
	/// Main update loop when the game has finished loading.
	/// </summary>
	void HandlePostGameLoadedMods(GameLoopState& state) 
	{
		GameState::currentMenu = GameState::GetCurrentMenu(); // This loads without checking if memory is safe... This can cause crashes if used when GameLoaded is false.

		HandleLinearRiffRepeaterToggle();
		HandleMidiDeviceScanning();

		GameState::LessonMode = GameState::Menus::IsInLessonModes();

		if (GameState::IsInSong()) {
			HandleInSongState(state);
		}
		else {
			HandleInMenuState(state);
		}

		HandleAlwaysOnMods(state);
	}

	/// <summary>
	/// Handles all state updates when the player is in menus.
	/// </summary>
	void HandleInMenuState(GameLoopState& state) {
		CleanupSongSpecificStates(state);
		D3DHooks::UpdateHeadstockCacheForMenu();

		GameState::previousMenu = GameState::currentMenu;
	}

	/// <summary>
	/// Cleans up states that are only active during songs.
	/// </summary>
	void CleanupSongSpecificStates(GameLoopState& state) {
		if (Settings::IsOn("AllowLooping")) {
			Keybindings::loopStart = NULL;
			Keybindings::loopEnd = NULL;
		}

		if (!GameState::Menus::IsInScoreMenus() && RiffRepeater::currentlyEnabled_Above100) {
			RiffRepeater::DisableTimeStretch();
		}

		if ((Midi::alreadyAutomatedTuningInThisSong || Midi::alreadyAttemptedTuningInTuner) &&
			!GameState::Menus::IsInPreSongTuner()) {
			Midi::RevertAutomatedTuning();
			Midi::alreadyAttemptedTuningInTuner = false;
			Midi::userWantsToUseAutoTuning = false;
		}
	}

	/// <summary>
	/// Handles updates while the game is still loading.
	/// </summary>
	void UpdateGameLoadingState(GameLoopState& state) {
		GameState::currentMenu = GameState::GetCurrentMenu(true); 	// This is the safe version of checking the current menu. It is only used while the game boots, else the game may crash.

		CheckIfGameHasLoaded();
		ConfigureAlternativeSampleRate();
		HandleAutoLoadProfile(state);
	}

	/// <summary>
	/// Configures buffer settings for alternative sample rates.
	/// </summary>
	void ConfigureAlternativeSampleRate() {
		if (Settings::IsOn("AltOutputSampleRate") &&
			Settings::GetModSetting("AlternativeOutputSampleRate") != 48000 &&
			*(int*)Offsets::ptr_sampleRateBuffer.Get() != 5 &&
			*(int*)Offsets::ptr_sampleRateBuffer.Get() != 2) {
			*(int*)Offsets::ptr_sampleRateSize.Get() = 2;
			*(int*)Offsets::ptr_sampleRateBuffer.Get() = 128;
		}
	}

	/// <summary>
	/// Checks if the game has finished loading or has the user opened a new user profile?
	/// This prevents the user from being locked in a loop.
	/// </summary>
	void CheckIfGameHasLoaded() {
		if (!GameState::GameLoaded && (GameState::currentMenu == "MainMenu" ||
			GameState::currentMenu == "PlayedRS1Select" ||
			GameState::currentMenu == "SimpleDialog")) {
			GameState::GameLoaded = true;
		}
	}

	/// <summary>
	/// Handles all state updates when the player is in a song.
	/// </summary>
	void HandleInSongState(GameLoopState& state) {
		LogSongIDForRiffRepeater();
		EnableRiffRepeaterFeatures();
		HandleMidiAutoTuningInSong();
	}

	/// <summary>
	/// Handles automatic profile loading (AKA "Fork in the toaster" mod).
	/// </summary>
	void HandleAutoLoadProfile(GameLoopState& state) {
		if (!Settings::IsOn("ForceProfileEnabled") ||
			GameState::Menus::IsInMenusWithDisallowedAutoEnter() ||
			state.forkInToasterNewProfile) {
			return;
		}

		// Skip UPlay login dialog - depending on the menu it might be necessary to press either ESC or Enter, so just spam both
		if (GameState::currentMenu == "SelectionListDialog" ||
			GameState::currentMenu == "UplayLoginDialog") {
			Keyboard::SendEscapeKey();
			Keyboard::AutoEnterGame();
		}
		else if (Settings::ReturnSettingValue("ProfileToLoad") != "" &&
			GameState::currentMenu == "ProfileSelect") { // If the user user says "I want to always load this profile"
			HandleSpecificProfileLoad(state);
		}
		else { 	// User doesn't care what profile we select, just select the first / top one.
			Keyboard::AutoEnterGame();
		}
	}

	/// <summary>
	/// Loads a specific user profile by name.
	/// </summary>
	void HandleSpecificProfileLoad(GameLoopState& state) {
		state.selectedUser = GameState::CurrentSelectedUser();

		if (state.selectedUser == Settings::ReturnSettingValue("ProfileToLoad")) {
			Keyboard::AutoEnterGame();
		}
		else if (state.selectedUser == "New profile") {
			LOG_ERROR("(Auto Load) Invalid Profile Name" << std::endl); // Yeah, the profile they're looking for doesn't exist :(
			state.forkInToasterNewProfile = true;
		}
		else { // Not the profile we're looking for. Move down 1.
			Keyboard::PressDownArrowKey();
		}
	}

	/// <summary>
	/// Logs song ID for Riff Repeater > 100% functionality.
	/// We are in a song we haven't seen in this play session. Log its Id so we can prep for the Riff Repeater > 100% mod.
	/// </summary>
	void LogSongIDForRiffRepeater() {
		if (RiffRepeater::readyToLogSongID && RiffRepeater::LogSongID(GameState::GetSongKey())) {	
			RiffRepeater::readyToLogSongID = false;
		}
	}

	/// <summary>
	/// Enables riff repeater time stretching features.
	/// </summary>
	void EnableRiffRepeaterFeatures() {
		if (Settings::IsOn("RRSpeedAboveOneHundred")) {
			RiffRepeater::EnableTimeStretch();
		}
	}


	/// <summary>
	/// Handles MIDI auto-tuning when entering a song.
	/// </summary>
	void HandleMidiAutoTuningInSong() {
		if (Settings::IsOn("AutoTuneForSong") &&
			!Midi::alreadyAutomatedTuningInThisSong &&
			(Settings::GetWhen("AutoTuneForSongWhen") == When::Tuner ||
				(Settings::GetWhen("AutoTuneForSongWhen") == When::Manual && Midi::userWantsToUseAutoTuning))) {
			Midi::AutomateTuning();
		}
	}

	/// <summary>
	/// Shows or hides the song timer based on settings.
	/// </summary>
}
