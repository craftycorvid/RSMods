#pragma once
namespace Menu
{
	inline bool ImGuiInit = false;

	void Init(IDirect3DDevice9* pDevice, LONG_PTR WndProc);
	void AddMidiMenu();
	void AddMicrophonesMenu();
	void AddVoicelinesMenu();
	void AddCalibrationMenu();
	bool IsOverlayCall();
	void UpdateStringTextures(IDirect3DDevice9* pDevice);
	void RenderImGuiMenu();
	void CreateTextures(IDirect3DDevice9* pDevice);
};

