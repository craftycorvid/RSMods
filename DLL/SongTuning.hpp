#pragma once

#include "Windows.h"
#include "Structs.hpp"
#include "stdafx.h"
#include "Log.hpp"

namespace SongTuning {
	byte getLowestStringTuning();
	byte* GetCurrentTuning(bool verbose = false);
	bool IsExtendedRangeSong();
	int* GetHighestLowestString();
	int* GetHighestLowestString(Tuning tuningOverride);
	bool IsSongInDrop(Tuning tuning);
	bool IsSongInStandard(Tuning tuning);
	int GetTrueTuning();
	Tuning GetTuningAtTuner();
	bool IsExtendedRangeTuner();
};