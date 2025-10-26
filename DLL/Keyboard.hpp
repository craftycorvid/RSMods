#pragma once

#include <Windows.h>
#include "Log.hpp"
#include <iostream>

namespace Keyboard {
	inline bool takenScreenshotOfThisScreen = false; // Has the user taken a screenshot of their scores in this menu

	void PressDownArrowKey();
	void SendEscapeKey();
	void AutoEnterGame();
	void TakeScreenshot();
}