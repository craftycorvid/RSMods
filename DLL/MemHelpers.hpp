#pragma once

#include "Structs.hpp"

namespace Util {
	inline void SendKey(unsigned int key) {
		PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, key, 0);
		Sleep(30);
		PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, key, 0);
	}
};


