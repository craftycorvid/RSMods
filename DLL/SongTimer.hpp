#pragma once

#include "stdafx.h"

namespace SongTimer {
	float SongTimer();
	float GetGreyNoteTimer();
	void SetGreyNoteTimer(float timeInSeconds);
	double GetNonStopPlayTimer();
	void SetNonStopPlayTimer(double NewTimer);
}