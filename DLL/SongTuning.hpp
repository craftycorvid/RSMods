#pragma once

#include "stdafx.h"

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