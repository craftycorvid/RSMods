#pragma once

#include "Windows.h"
#include <cstring>

struct Tuning {
	// Absolutely unrealistic default values :)

	byte lowE = 69; //0x0000
	char    pad_0001 = 0; //0x0001
	byte strA = 69; //0x0002
	char    pad_0003 = 0; //0x0003
	byte strD = 69; //0x0004
	char    pad_0005 = 0; //0x0005
	byte strG = 69; //0x0006
	char    pad_0007 = 0; //0x0007
	byte strB = 69; //0x0008
	char    pad_0009 = 0; //0x0009
	byte highE = 69; //0x000A

	Tuning() = default;

	Tuning(byte _lowE, byte _strA, byte _strD, byte _strG, byte _strB, byte _highE)
		: lowE(_lowE),  
		strA(_strA),
		strD(_strD),
		strG(_strG),
		strB(_strB),
		highE(_highE)
	{}
};