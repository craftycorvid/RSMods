#pragma once

#include <Windows.h>
#include "Log.hpp"
#include <iostream>

namespace Keyboard {
	inline bool takenScreenshotOfThisScreen = false; // Has the user taken a screenshot of their scores in this menu

	void PressDownArrowKey()
	{
		PostMessage(FindWindow(nullptr, L"Rocksmith 2014"), WM_KEYDOWN, VK_DOWN, 0);
		Sleep(30);
		PostMessage(FindWindow(nullptr, L"Rocksmith 2014"), WM_KEYUP, VK_DOWN, 0);
	}

	void SendEscapeKey() {
		PostMessage(FindWindow(nullptr, L"Rocksmith 2014"), WM_KEYDOWN, VK_ESCAPE, 0);
		Sleep(30);
		PostMessage(FindWindow(nullptr, L"Rocksmith 2014"), WM_KEYUP, VK_ESCAPE, 0);
	}


	/// <summary>
	/// Presses Enter. Normally used in a loop to skip most of the login dialog. "Fork in the toaster" method
	/// </summary>
	void AutoEnterGame() {
		PostMessage(FindWindow(nullptr, L"Rocksmith 2014"), WM_KEYDOWN, VK_RETURN, 0);
		Sleep(30);
		PostMessage(FindWindow(nullptr, L"Rocksmith 2014"), WM_KEYUP, VK_RETURN, 0);
	}

	/// <summary>
	/// Force a Steam screenshot. Requires the default "F12" screenshot key for Steam.
	/// </summary>
	void TakeScreenshot() {
		_LOG_INIT;
		if (!takenScreenshotOfThisScreen) {
			takenScreenshotOfThisScreen = true;
			Sleep(8000); // The menu title changes while the animation is running so we are giving it so time to show the actual results. (8 seconds)

			// Press F12
			PostMessage(FindWindow(nullptr, L"Rocksmith 2014"), WM_KEYDOWN, VK_F12, 0);
			_LOG("Took screenshot" << std::endl);
			Sleep(30);
			PostMessage(FindWindow(nullptr, L"Rocksmith 2014"), WM_KEYUP, VK_F12, 0);
		}
	}
}