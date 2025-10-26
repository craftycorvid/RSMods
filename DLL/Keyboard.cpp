#include "stdafx.h"
#include "Keyboard.hpp"

namespace Keyboard {
	/// Simply send ESC key as if pressed via the keyboard
	/// Used to avoid menus.
	/// </summary>
	void PressDownArrowKey()
	{
		PostMessage(FindWindow(nullptr, L"Rocksmith 2014"), WM_KEYDOWN, VK_DOWN, 0);
		Sleep(30);
		PostMessage(FindWindow(nullptr, L"Rocksmith 2014"), WM_KEYUP, VK_DOWN, 0);
	}

	/// <summary>
	/// Simply send ESC key as if pressed via the keyboard
	/// Used to avoid menus during startup.
	/// </summary>
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
		if (!takenScreenshotOfThisScreen) {
			takenScreenshotOfThisScreen = true;
			Sleep(8000); // The menu title changes while the animation is running so we are giving it so time to show the actual results. (8 seconds)

			// Press F12
			PostMessage(FindWindow(nullptr, L"Rocksmith 2014"), WM_KEYDOWN, VK_F12, 0);
			LOG_INFO("Took screenshot" << std::endl);
			Sleep(30);
			PostMessage(FindWindow(nullptr, L"Rocksmith 2014"), WM_KEYUP, VK_F12, 0);
		}
	}
}