#include "stdafx.h"
#include "ModManager.hpp"

namespace Setting = Settings::Setting;

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

		if (Settings::IsOn(Setting::FixBrokenTones)) {
			BugPrevention::PreventStuckTone();
		}

		if (Settings::IsOn(Setting::FixOculusCrash)) {
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

	/// <summary>
	/// Applies all mods and fixes that must run at startup.
	/// </summary>
	void ApplyStartupMods()
	{
		AudioDevices::SetupMicrophones();
		ApplyBugPrevention();

		#ifdef _WWISE_LOGS
				Wwise::Logging::Init();
		#endif
	}

	/// <summary>
	/// Per-tick host game-state upkeep once the game has loaded, run before the mod registry ticks. All the
	/// song-side work this used to fan out to now lives in mods; only menu-side host upkeep remains here.
	/// </summary>
	void HandlePostGameLoadedMods()
	{
		GameState::currentMenu = GameState::GetCurrentMenu(); // This loads without checking if memory is safe... This can cause crashes if used when GameLoaded is false.
		GameState::LessonMode = GameState::Menus::IsInLessonModes();

		if (GameState::IsInSong())
			return;

		// Returning to a menu ends the song: drop the A/B loop markers so the next song starts fresh.
		if (Settings::IsOn(Setting::AllowLooping)) {
			Keybindings::loopStart = NULL;
			Keybindings::loopEnd = NULL;
		}

		D3DHooks::UpdateHeadstockCacheForMenu();
		GameState::previousMenu = GameState::currentMenu;
	}

	/// <summary>
	/// Refreshes host game state while the game is still loading, before the mod registry ticks.
	/// </summary>
	void UpdateGameLoadingState() {
		GameState::currentMenu = GameState::GetCurrentMenu(true); 	// This is the safe version of checking the current menu. It is only used while the game boots, else the game may crash.

		CheckIfGameHasLoaded();
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
}
