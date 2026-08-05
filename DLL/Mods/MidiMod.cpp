#include "../stdafx.h"
#include "MidiMod.hpp"
#include "Midi.hpp"

using Framework::ModContext;
using Framework::GamePhase;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Settings::When;

std::string_view MidiMod::Id() const {
	return "Midi";
}

std::vector<std::string_view> MidiMod::ClaimsExclusive() const {
	return { "tuning-controller" };
}

void MidiMod::OnInitialize(ModContext& c) {
	// This command changes tuning-controller state, so it must never bypass conflict suppression.
	c.Commands().BindSetting(
		"TuningOffsetKey",
		KeyEdge::Up,
		Availability::Active,
		[](ModContext&, const KeyEvent& event) {
			Midi::tuningOffset += event.control ? -1 : 1;
			Midi::tuningOffset = std::clamp(Midi::tuningOffset, -3, 12);
			LOG_INFO("Triggered Mod Setting: Tuning Offset is now set to " << Midi::tuningOffset << std::endl);
		},
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn("AutoTuneForSong");
		});

	// Delete is a fixed host shortcut, but its intent belongs to this tuning-controller mod.
	c.Commands().BindKey(
		"ManualMidiAutoTune",
		VK_DELETE,
		KeyEdge::Up,
		Availability::Active,
		[](ModContext&, const KeyEvent&) {
			Midi::userWantsToUseAutoTuning = true;
		},
		[](const ModContext& context, const KeyEvent&) {
			return context.WhenSetting("AutoTuneForSongWhen") == When::Manual &&
				GameState::Menus::IsInTuningMenus();
		});
}

void MidiMod::OnTick(ModContext& c) {
	if (c.phase == GamePhase::Loading)
		return;

	ScanForMidiDevices(c);
}

// Two independent one-time setup steps: load the auto-tune pedal settings once AutoTuneForSong is on, 
// and spin up the MIDI-in listener once a MidiInDevice is configured.
void MidiMod::ScanForMidiDevices(ModContext& c) {
	if (!Midi::scannedForMidiDevices && c.IsOn("AutoTuneForSong")) {
		Midi::scannedForMidiDevices = true;
		Midi::ReadMidiSettingsFromINI(
			c.Value("ChordsMode"),
			c.Int("TuningPedal"),
			c.Value("AutoTuneForSongDevice"),
			c.Value("MidiInDevice"));
	}

	if (!Midi::attemptedToDetachMidiInThread && c.Value("MidiInDevice") != "") {
		Midi::attemptedToDetachMidiInThread = true;
		Midi::FindMidiInDevices(c.Value("MidiInDevice"));
		std::thread(Midi::ListenToMidiInThread).detach();
	}
}

// In menus. Revert first, then auto-tune if we are sitting in the pre-song tuner,
// preserving the original revert-before-tuner ordering.
void MidiMod::OnMenuTick(ModContext& c) {
	RevertTuningWhenLeavingSong();
	AutoTuneInTuner(c);
}

// Once we have left the song (and are not in its tuner), tell the pedal to drop the tuning we applied.
void MidiMod::RevertTuningWhenLeavingSong() {
	if ((Midi::alreadyAutomatedTuningInThisSong || Midi::alreadyAttemptedTuningInTuner) &&
		!GameState::Menus::IsInPreSongTuner()) {
		Midi::RevertAutomatedTuning();
		Midi::alreadyAttemptedTuningInTuner = false;
		Midi::userWantsToUseAutoTuning = false;
	}
}

// While the pre-song tuner is up, tune the pedal from the tuner's tuning readout (When == Tuner only).
void MidiMod::AutoTuneInTuner(ModContext& c) {
	if (GameState::Menus::IsInPreSongTuner() &&
		c.IsOn("AutoTuneForSong") &&
		c.WhenSetting("AutoTuneForSongWhen") == When::Tuner &&
		!Midi::alreadyAttemptedTuningInTuner &&
		!Midi::alreadyAutomatedTuningInThisSong) {
		Midi::AttemptTuningInTuner();
		Midi::appliedTunerAutoTune = true; // Lets ExtendedRange skip its tuning-settle sleep.
	}
}

// In a song. Tune the pedal from the song's tuning the first time through, honouring the When setting.
void MidiMod::OnSongTick(ModContext& c) {
	AutoTuneInSong(c);
}

void MidiMod::AutoTuneInSong(ModContext& c) {
	if (c.IsOn("AutoTuneForSong") &&
		!Midi::alreadyAutomatedTuningInThisSong &&
		(c.WhenSetting("AutoTuneForSongWhen") == When::Tuner ||
			(c.WhenSetting("AutoTuneForSongWhen") == When::Manual && Midi::userWantsToUseAutoTuning))) {
		Midi::AutomateTuning();
	}
}

static Framework::ModRegistrar<MidiMod> _midiReg;
