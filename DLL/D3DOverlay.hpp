#pragma once

#include "stdafx.h"

namespace GameOverlay {
	Resolution GetWindowSize();
	void DX9DrawText(const std::string& textToDraw, int textColorHex, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, LPDIRECT3DDEVICE9 pDevice, Resolution setFontSize = { NULL, NULL }, DWORD format = DT_LEFT | DT_NOCLIP);

	inline HRESULT CustomDX9Font = NULL;
	inline ID3DXFont* DX9FontEncapsulation = NULL;
	inline int fontWidth = NULL, fontHeight = NULL;
}