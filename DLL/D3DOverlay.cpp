#include "stdafx.h"
#include "D3DOverlay.hpp"

/// <returns>Size of Rocksmith Window</returns>
Resolution GameOverlay::GetWindowSize() {
	RECT WindowSize;

	Resolution currentSize;
	if (GetWindowRect(FindWindow(NULL, L"Rocksmith 2014"), &WindowSize))
	{
		currentSize.width = WindowSize.right - WindowSize.left;
		currentSize.height = WindowSize.bottom - WindowSize.top;
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
	Resolution WindowSize = GameOverlay::GetWindowSize();

	// Allow Font Size Declaration
	bool useInputFontSize = (setFontSize.width != NULL && setFontSize.height != NULL);

	// If the user changes resolutions, we want to scale the text dynamically. This also covers the first font creation as the font and fontSize variables are all null to begin with.
	if ((fontWidth != (WindowSize.width / 96) || fontHeight != (WindowSize.height / 72) || CustomDX9Font == NULL) && !useInputFontSize) {
		fontWidth = (WindowSize.width / 96);
		fontHeight = (WindowSize.height / 72);

		CustomDX9Font = D3DXCreateFontA(pDevice,
			fontWidth,
			fontHeight,
			FW_NORMAL,
			1,
			false,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			ANTIALIASED_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE,
			Settings::ReturnSettingValue("OnScreenFont").c_str(),
			&DX9FontEncapsulation); // Create a new font
	}
	else if (useInputFontSize)
		CustomDX9Font = D3DXCreateFontA(pDevice,
			setFontSize.width,
			setFontSize.height,
			FW_NORMAL,
			1,
			false,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			ANTIALIASED_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE,
			Settings::ReturnSettingValue("OnScreenFont").c_str(),
			&DX9FontEncapsulation); // Create a new font

	// Create Area To Draw Text
	RECT TextRectangle{ topLeftX, topLeftY, bottomRightX, bottomRightY }; // Left, Top, Right, Bottom

	// Preload And Draw The Text (Supposed to reduce the performance hit (It's D3D/DX9 but still good practice))
	DX9FontEncapsulation->PreloadTextA(textToDraw.c_str(), textToDraw.length());
	DX9FontEncapsulation->DrawTextA(NULL, textToDraw.c_str(), -1, &TextRectangle, format, textColorHex);

	// Let's clean up our junk, since fonts don't do it automatically.
	if (DX9FontEncapsulation) {
		DX9FontEncapsulation->Release();
		DX9FontEncapsulation = NULL;
	}
}