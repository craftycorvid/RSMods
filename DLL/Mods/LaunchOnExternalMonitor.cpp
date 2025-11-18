#include "../stdafx.h"
#include "LaunchOnExternalMonitor.hpp"

namespace LaunchOnExternalMonitor {
	/// <summary>
	/// Move Rocksmith to a separate monitor on boot.
	/// </summary>
	/// <param name="startX"> - top LEFT of the screen</param>
	/// <param name="startY"> - TOP left of the screen</param>
	void SendRocksmithToScreen(int startX, int startY) {
		HWND hWnd = D3DHooks::GetGameWindow();
	
		while (!hWnd) {
			Sleep(500);
			hWnd = FindWindowA(nullptr, "Rocksmith 2014");
		}
		
		// Set the windows top left corner to StartX and StartY.
		RECT windowSize;
		if (GetWindowRect(hWnd, &windowSize)) {
			SetWindowPos(hWnd, HWND_TOP, startX, startY, windowSize.right - windowSize.left, windowSize.bottom - windowSize.top, SWP_SHOWWINDOW);
		}
	}
}
