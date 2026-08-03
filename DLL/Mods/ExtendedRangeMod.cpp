#include "../stdafx.h"
#include "ExtendedRangeMod.hpp"
#include "../ModManager.hpp"
#include "ExtendedRangeMode.hpp"

using Framework::ModContext;
using Settings::StringColorMode;
using Settings::NoteColorMode;

std::string_view ExtendedRangeMod::Id() const {
	return "ExtendedRange";
}

// Edge: this mod became active in a song.
void ExtendedRangeMod::OnSongEnter(ModContext& c) {
	// Tuner detection must not bleed into the song.
	ERMode::AttemptedERInTuner = false;
	ERMode::UseERInTuner = false;

	if (ERMode::AttemptedERInThisSong) return;

	// Tuning takes ~1.5s to settle before the numbers are trustworthy.
	if (!SkipERSleep(c)) Sleep(1500); 	

	ERMode::UseERExclusivelyInThisSong = SongTuning::IsExtendedRangeSong();
	ERMode::UseEROrColorsInThisSong =
		(c.IsOn("ExtendedRangeEnabled") && ERMode::UseERExclusivelyInThisSong) ||
		c.ColorModeSetting("CustomStringColors") == StringColorMode::Custom ||
		(c.IsOn("SeparateNoteColors") && c.NoteColorModeSetting("SeparateNoteColorsMode") != NoteColorMode::Default);
	ERMode::AttemptedERInThisSong = true;
}

void ExtendedRangeMod::OnSongTick(ModContext&) {
	ApplyColors();
}

void ExtendedRangeMod::OnMenuTick(ModContext& c) {
	if (GameState::Menus::IsInPreSongTuner()) {
		if (!ERMode::AttemptedERInTuner) {
			if (!SkipERSleep(c)) Sleep(1500);

			ERMode::AttemptedERInTuner = true;
			ERMode::UseERInTuner = SongTuning::IsExtendedRangeTuner();
		}
	}
	else {
		ERMode::AttemptedERInTuner = false;
		ERMode::UseERInTuner = false;
	}

	ApplyColors();
}

// Edge: this mod stopped being active in a song. Formerly the ER block of
// CleanupSongSpecificStates.
void ExtendedRangeMod::OnSongExit(ModContext&) {
	if (ERMode::AttemptedERInThisSong) {
		ERMode::UseERExclusivelyInThisSong = false;
		ERMode::UseEROrColorsInThisSong = false;
		ERMode::AttemptedERInThisSong = false;
	}
}

// Always-on string coloring. Runs at the END of each phase tick so the flag updates above are visible to 
// Toggle7StringMode in the same frame. 
// Old debt: DoRainbow() intentionally blocks MainThread until rainbow is toggled off
void ExtendedRangeMod::ApplyColors() {
	if (ERMode::IsRainbowEnabled() || ERMode::IsRainbowNotesEnabled())
		ERMode::DoRainbow();
	else
		ERMode::Toggle7StringMode();
}

// skipERSleep still lives in the transitional GameLoopState - it is set by the
// MIDI auto-tune-in-tuner path, which is not ported yet.
bool ExtendedRangeMod::SkipERSleep(const ModContext& c) {
	return c.loop && c.loop->skipERSleep;
}

static Framework::ModRegistrar<ExtendedRangeMod> _extendedRangeReg;
