#include "../stdafx.h"
#include "RiffRepeaterMod.hpp"

using Framework::ModContext;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;

std::string_view RiffRepeaterMod::Id() const {
	return "RiffRepeater";
}

void RiffRepeaterMod::OnInitialize(ModContext& c) {
	c.Commands().BindSetting(
		"RewindKey",
		KeyEdge::Up,
		Availability::Active,
		[](ModContext& context, const KeyEvent&) { Rewind(context); },
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn("AllowRewind") && GameState::Menus::IsInLASPlayingModes();
		});

	c.Commands().BindSetting(
		"LoopStartKey",
		KeyEdge::Down,
		Availability::Active,
		[](ModContext&, const KeyEvent& event) { SetLoopStart(event); },
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn("AllowLooping") && GameState::Menus::IsInModesWithAllowedFastRiffRepeater();
		},
		"Loop Start Point Set");

	c.Commands().BindSetting(
		"LoopEndKey",
		KeyEdge::Down,
		Availability::Active,
		[](ModContext&, const KeyEvent& event) { SetLoopEnd(event); },
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn("AllowLooping") && GameState::Menus::IsInModesWithAllowedFastRiffRepeater();
		},
		"Loop End Point Set");

	c.Commands().BindSetting(
		"RRSpeedKey",
		KeyEdge::Down,
		Availability::Active,
		[](ModContext& context, const KeyEvent& event) { ChangeSpeed(context, event); },
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn("RRSpeedAboveOneHundred") &&
				GameState::Menus::IsInModesWithAllowedFastRiffRepeater() &&
				RiffRepeater::loggedCurrentSongID;
		});
}

void RiffRepeaterMod::Rewind(const ModContext& c) {
	auto seekTo = static_cast<AkTimeMs>((SongTimer::SongTimer() * 1000) - c.Int("RewindBy") - c.Int("RewindLeadup"));
	if (seekTo < 0) seekTo = 0;
	Wwise::SoundEngine::SeekOnEvent(std::string("Play_" + GameState::GetSongKey()).c_str(), 0x1234, seekTo, false);

	const AkTimeMs greyNoteTimerMs = seekTo - c.Int("RewindLeadup");
	SongTimer::SetGreyNoteTimer(greyNoteTimerMs / 1000.f);

	LOG_INFO("(REWIND) Seeked to " << seekTo << "ms." << std::endl);
}

void RiffRepeaterMod::SetLoopStart(const KeyEvent& event) {
	if (event.control) {
		Keybindings::loopStart = Keybindings::loopEnd = NULL;
		return;
	}

	Keybindings::loopStart = SongTimer::SongTimer();
	if (Keybindings::loopEnd <= Keybindings::loopStart) Keybindings::loopEnd = NULL;
}

void RiffRepeaterMod::SetLoopEnd(const KeyEvent& event) {
	if (event.control) {
		Keybindings::loopEnd = NULL;
		return;
	}

	Keybindings::loopEnd = SongTimer::SongTimer();
	if (Keybindings::loopEnd <= Keybindings::loopStart) Keybindings::loopEnd = NULL;
}

void RiffRepeaterMod::ChangeSpeed(const ModContext& c, const KeyEvent& event) {
	const float interval = static_cast<float>(c.Int("RRSpeedInterval"));

	float realSongSpeed = RiffRepeater::GetSpeed(true);
	realSongSpeed += event.control ? -interval : interval;
	realSongSpeed = std::clamp(realSongSpeed, 25.f, 400.f);

	RiffRepeater::SetSpeed(realSongSpeed, true);
	RiffRepeater::EnableTimeStretch();
	RiffRepeater::saveNewRRSpeedToFile = true;

	LOG_INFO("Triggered Mod: Song Speed set to " << realSongSpeed << "%" << std::endl);
}

void RiffRepeaterMod::OnTick(ModContext& c) {
	SyncLinearSpeeds(c);
}

// Patch (or revert) the linear Riff Repeater speed logic to track the setting.
void RiffRepeaterMod::SyncLinearSpeeds(ModContext& c) {
	if (c.IsOn("LinearRiffRepeater") && !RiffRepeater::currentlyEnabled_LinearRR) {
		RiffRepeater::EnableLinearSpeeds();
	}
	else if (c.IsOff("LinearRiffRepeater") && RiffRepeater::currentlyEnabled_LinearRR) {
		RiffRepeater::DisableLinearSpeeds();
	}
}

// Leaving a song for any menu other than the score screens drops the >100% time stretch.
void RiffRepeaterMod::OnMenuTick(ModContext& c) {
	if (!GameState::Menus::IsInScoreMenus() && RiffRepeater::currentlyEnabled_Above100) {
		RiffRepeater::DisableTimeStretch();
	}
}

void RiffRepeaterMod::OnSongTick(ModContext& c) {
	// First time we see this song, log its id so the >100% speed table can prep.
	if (RiffRepeater::readyToLogSongID && RiffRepeater::LogSongID(GameState::GetSongKey())) {
		RiffRepeater::readyToLogSongID = false;
	}

	if (c.IsOn("RRSpeedAboveOneHundred")) {
		RiffRepeater::EnableTimeStretch();
	}
}

static Framework::ModRegistrar<RiffRepeaterMod> _riffRepeaterReg;
