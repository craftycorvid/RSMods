#include "../stdafx.h"
#include "ExtendedRangeMod.hpp"
#include "Midi.hpp"
#include "ExtendedRangeMode.hpp"

using Framework::ModContext;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Settings::StringColorMode;
using Settings::NoteColorMode;
namespace Setting = Settings::Setting;

void ExtendedRangeMod::OnInitialize(ModContext& c) {
	c.Commands().BindSetting(
		"RainbowStringsKey",
		KeyEdge::Up,
		Availability::Active,
		[](ModContext&, const KeyEvent&) {
			ERMode::ToggleRainbowMode();
			if (!ERMode::RainbowEnabled) ERMode::ResetAllStrings();
		},
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn(Setting::RainbowStringsEnabled);
		},
		"Rainbow Strings");

	c.Commands().BindSetting(
		"ToggleExtendedRangeKey",
		KeyEdge::Up,
		Availability::Active,
		[](ModContext&, const KeyEvent&) {
			ERMode::UseERExclusivelyInThisSong = !ERMode::UseERExclusivelyInThisSong;
			GameState::ToggleCB(ERMode::UseERExclusivelyInThisSong);
		},
		{},
		"Toggle Extended Range");
}

void ExtendedRangeMod::OnShutdown(ModContext&) {
	ERMode::StopRainbowThread();
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
		(c.IsOn(Setting::ExtendedRangeEnabled) && ERMode::UseERExclusivelyInThisSong) ||
		c.ColorMode() == StringColorMode::Custom ||
		(c.IsOn(Setting::SeparateNoteColors) && c.NoteColorMode() != NoteColorMode::Default);
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
// DoRainbow() launches the rainbow animation on its own thread (it must not block MainThread, which
// also drains keybind commands, otherwise the effect could never be toggled back off).
void ExtendedRangeMod::ApplyColors() {
	if (ERMode::IsRainbowEnabled() || ERMode::IsRainbowNotesEnabled())
		ERMode::DoRainbow();
	else
		ERMode::Toggle7StringMode();
}

// Skip the 1.5s tuning-settle sleep once MIDI has auto-tuned in the pre-song tuner: the pedal tuning is
// already applied and the game's tuning numbers are stable. MidiMod owns and latches this flag.
bool ExtendedRangeMod::SkipERSleep(const ModContext&) {
	return Midi::appliedTunerAutoTune;
}

static Framework::ModRegistrar<ExtendedRangeMod> _extendedRangeReg;
