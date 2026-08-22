#include "stdafx.h"
#include "Keyboard.hpp"

namespace Keyboard {
	/// Simply send ESC key as if pressed via the keyboard
	/// Used to avoid menus.
	/// </summary>
	void PressDownArrowKey()
	{
		PostMessage(D3DHooks::GetGameWindow(), WM_KEYDOWN, VK_DOWN, 0);
		Sleep(30);
		PostMessage(D3DHooks::GetGameWindow(), WM_KEYUP, VK_DOWN, 0);
	}

	/// <summary>
	/// Simply send ESC key as if pressed via the keyboard
	/// Used to avoid menus during startup.
	/// </summary>
	void SendEscapeKey() {
		PostMessage(D3DHooks::GetGameWindow(), WM_KEYDOWN, VK_ESCAPE, 0);
		Sleep(30);
		PostMessage(D3DHooks::GetGameWindow(), WM_KEYUP, VK_ESCAPE, 0);
	}

	/// <summary>
	/// Presses Enter. Normally used in a loop to skip most of the login dialog. "Fork in the toaster" method
	/// </summary>
	void AutoEnterGame() {
		PostMessage(D3DHooks::GetGameWindow(), WM_KEYDOWN, VK_RETURN, 0);
		Sleep(30);
		PostMessage(D3DHooks::GetGameWindow(), WM_KEYUP, VK_RETURN, 0);
	}

	/// <summary>
	/// Force a Steam screenshot. Requires the default "F12" screenshot key for Steam.
	/// </summary>
	void TakeScreenshot() {
		PostMessage(D3DHooks::GetGameWindow(), WM_KEYDOWN, VK_F12, 0);
		LOG_INFO("Took screenshot" << std::endl);
		Sleep(30);
		PostMessage(D3DHooks::GetGameWindow(), WM_KEYUP, VK_F12, 0);
	}
}
