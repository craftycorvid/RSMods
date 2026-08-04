#include "../stdafx.h"
#include "RiffRepeaterMod.hpp"
#include "RiffRepeater.hpp"

using Framework::ModContext;

std::string_view RiffRepeaterMod::Id() const {
	return "RiffRepeater";
}

void RiffRepeaterMod::OnTick(ModContext& c) {
	SyncLinearSpeeds(c);
}

// Patch (or revert) the linear Riff Repeater speed logic to track the setting. Runs in every phase so the
// patch lands as early as the old startup pass did, then stays in sync if the user toggles it later.
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
