#include "stdafx.h"
#include "ModManager.hpp"
#include "Mods/Midi.hpp"

namespace Setting = Settings::Setting;

namespace {
	typedef IDirect3D9* (WINAPI* tDirect3DCreate9)(UINT SDKVersion);

	/// <summary>
	/// Read the IDirect3DDevice9 vTable out of a throwaway device.
	/// Every device handed out by a given d3d9.dll shares one vTable, so the entries we find here are
	/// the ones the game's real device calls through. Works the same on Microsoft's d3d9, WineD3D, and DXVK.
	/// </summary>
	/// <param name="d3d9Module"> - Handle of the d3d9.dll the game loaded.</param>
	/// <returns>The device vTable, or NULL if we couldn't create a device to read it from.</returns>
	void** GetD3D9DeviceVTable(HMODULE d3d9Module) {
		// Resolve the entry point out of the module the game already loaded, rather than importing it.
		// That keeps us from pulling d3d9.dll into the process earlier than the game would itself.
		tDirect3DCreate9 direct3DCreate9 = (tDirect3DCreate9)GetProcAddress(d3d9Module, "Direct3DCreate9");

		if (!direct3DCreate9) {
			LOG_ERROR("d3d9.dll does not export Direct3DCreate9." << std::endl);
			return NULL;
		}

		IDirect3D9* d3d9 = direct3DCreate9(D3D_SDK_VERSION);

		if (!d3d9) {
			LOG_ERROR("Direct3DCreate9 failed." << std::endl);
			return NULL;
		}

		D3DPRESENT_PARAMETERS presentParameters{};
		presentParameters.Windowed = TRUE;
		presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
		presentParameters.hDeviceWindow = GetDesktopWindow();

		IDirect3DDevice9* dummyDevice = NULL;
		HRESULT result = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, presentParameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParameters, &dummyDevice);

		// DXVK's NULLREF support is unreliable, so it is only worth trying once HAL has already failed.
		if (FAILED(result)) {
			LOG_WARNING("Could not create a HAL device (0x" << std::hex << result << std::dec << "). Retrying with NULLREF." << std::endl);
			result = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, presentParameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParameters, &dummyDevice);
		}

		void** vTable = NULL;

		if (SUCCEEDED(result) && dummyDevice) {
			vTable = *(void***)dummyDevice;
			dummyDevice->Release();
		}
		else
			LOG_ERROR("Could not create a D3D9 device to read the vTable from. Error: 0x" << std::hex << result << std::dec << std::endl);

		d3d9->Release();

		return vTable;
	}

	/// <summary>
	/// Point a vTable entry at one of our hooks, handing back the entry we replaced so the hook can chain to it.
	/// One pointer write, so it makes no assumptions about what the function it replaces looks like.
	/// </summary>
	/// <param name="vTable"> - Device vTable.</param>
	/// <param name="index"> - Index of the function to hook (see D3DInfo).</param>
	/// <param name="hook"> - Our replacement function.</param>
	/// <param name="original"> - Receives the function we replaced.</param>
	/// <returns>Was the hook installed?</returns>
	template <typename TOriginal, typename THook>
	bool HookVTableEntry(void** vTable, int index, THook hook, TOriginal& original) {
		DWORD oldProtection;

		// The vTable lives in read-only data, so it has to be made writable for the one pointer we swap.
		NTSTATUS status = MemUtil::HookedVirtualProtect(&vTable[index], sizeof(void*), PAGE_READWRITE, oldProtection);
		if (!NT_SUCCESS(status)) {
			LOG_ERROR("Could not unprotect the vTable entry at index " << index << ". Status: 0x" << std::hex << status << std::dec << std::endl);
			return false;
		}

		original = (TOriginal)vTable[index];
		vTable[index] = (void*)hook;

		DWORD backup;
		MemUtil::HookedVirtualProtect(&vTable[index], sizeof(void*), oldProtection, backup);

		return true;
	}
}

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
		BugPrevention::FixCalibrationSampleCount();

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
		HMODULE d3d9Module;

		// Wait for the game to load its Direct3D 9 implementation.
		while ((d3d9Module = GetModuleHandleA("d3d9.dll")) == NULL)
			Sleep(500);

		void** vTable = GetD3D9DeviceVTable(d3d9Module);

		if (!vTable) {
			LOG_ERROR("Could not find D3D device's vTable address." << std::endl);
			MessageBoxA(NULL, "Could not find D3D device's vTable address \n Restart the game and if you still get this error after a few tries, please report the error!", "Error", NULL);
			return;
		}

		// Hook D3D functions to use for our own D3D work. Reference D3DHooks
		HookVTableEntry(vTable, D3DInfo::SetVertexDeclaration_Index, D3DHooks::Hook_SetVertexDeclaration, oSetVertexDeclaration); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexdeclaration
		HookVTableEntry(vTable, D3DInfo::SetVertexShaderConstantF_Index, D3DHooks::Hook_SetVertexShaderConstantF, oSetVertexShaderConstantF); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexshaderconstantf
		HookVTableEntry(vTable, D3DInfo::Reset_Index, D3DHooks::Hook_Reset, oReset); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-reset
		HookVTableEntry(vTable, D3DInfo::SetVertexShader_Index, D3DHooks::Hook_SetVertexShader, oSetVertexShader); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexshader
		HookVTableEntry(vTable, D3DInfo::SetPixelShader_Index, D3DHooks::Hook_SetPixelShader, oSetPixelShader); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setpixelshader
		HookVTableEntry(vTable, D3DInfo::SetStreamSource_Index, D3DHooks::Hook_SetStreamSource, oSetStreamSource); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setstreamsource
		HookVTableEntry(vTable, D3DInfo::EndScene_Index, D3DHooks::Hook_EndScene, oEndScene); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-endscene
		HookVTableEntry(vTable, D3DInfo::DrawIndexedPrimitive_Index, D3DHooks::Hook_DIP, oDrawIndexedPrimitive); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawindexedprimitive
		HookVTableEntry(vTable, D3DInfo::DrawPrimitive_Index, D3DHooks::Hook_DP, oDrawPrimitive); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawprimitive
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
