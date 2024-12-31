#include "stdafx.h"
#include "SongTimer.hpp"

float SongTimer::SongTimer() {
	_LOG_INIT;

	_LOG_SETLEVEL(LogLevel::Error);

	if (GameState::Menus::IsInPreSongTuner()) {
		return 0.f;
	}

	uintptr_t addrTimerBase = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerBaseOffsets);
	uintptr_t addrTimerRare = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timerRare, Offsets::ptr_timerRareOffsets, true);

	// Null Pointer Check
	if (!addrTimerBase) {
		_LOG("Invalid Pointer: (BASE) ShowSongTimer" << std::endl);
		return 0.f;
	}

	// Null Pointer Check
	// At this point, we can verify that the timer is a valid time.
	if (!addrTimerRare) {
		_LOG("Invalid Pointer: (RARE) ShowSongTimer" << std::endl);
		return *(float*)addrTimerBase;
	}

	// We entered a song where the base timer does not work.
	// Cause for this is unknown but we need to check, or time based mods (looping, song timer) will break.
	// Ex: Desolate Motion, or Rocksmith 2012 Theme.
	if (GameState::Menus::IsInSongModes() && *(float*)addrTimerBase == 0.f && *(float*)addrTimerRare != 0.f) {
		return *(float*)addrTimerRare;
	}
	// This is the default case, and will be used 99.99% of the time.
	else {
		return *(float*)addrTimerBase;
	}
}

/// <summary>
/// When the user passes a note in the pause menu, notes become grey. This gets the time at which notes go from being grey (deactivated) to being colored (activated).
/// </summary>
/// <returns>Time where all notes before it are grey / deactivated.</returns>
float SongTimer::GetGreyNoteTimer() {
	_LOG_INIT;

	_LOG_SETLEVEL(LogLevel::Error);
	uintptr_t greyNoteTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_greyOutNoteTimer, Offsets::ptr_greyOutNoteTimerOffsets);

	if (!greyNoteTimer) {
		_LOG("Invalid Pointer: GetGreyNoteTimer = NULL" << std::endl);
		return NULL;
	}

	return *(float*)greyNoteTimer;
}

/// <summary>
/// Sets the time at which all notes before it turn grey (deactivated) and all notes after it are colorful (activated)
/// </summary>
/// <param name="timeInSeconds"> - Time to set the "deactivate before" at.</param>
void SongTimer::SetGreyNoteTimer(float timeInSeconds) {
	_LOG_INIT;

	_LOG_SETLEVEL(LogLevel::Error);
	uintptr_t greyNoteTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_greyOutNoteTimer, Offsets::ptr_greyOutNoteTimerOffsets);

	if (!greyNoteTimer) {
		_LOG("Invalid Pointer: SetGreyNoteTimer = NULL" << std::endl);
		return;
	}

	*(float*)greyNoteTimer = timeInSeconds;
}

/// <summary>
/// Gets how long the countdown will last for from start to end, between songs, in NSP.
/// </summary>
/// <returns>The amount of time it takes to go from song 1 to song 2.</returns>
double SongTimer::GetNonStopPlayTimer()
{
	return *(double*)Offsets::ptr_NonStopPlayPreSongTimer.Get();
}

/// <summary>
/// Sets the amount of time between song 1 and song 2 in NSP.
/// </summary>
/// <param name="NewTimer"> - New time to set</param>
void SongTimer::SetNonStopPlayTimer(double NewTimer)
{
	MemUtil::SetStaticValue(Offsets::ptr_NonStopPlayPreSongTimer, NewTimer, sizeof(NewTimer));
}
