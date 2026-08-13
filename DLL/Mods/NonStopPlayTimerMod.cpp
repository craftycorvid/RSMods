#include "../stdafx.h"
#include "NonStopPlayTimerMod.hpp"

using Framework::ModContext;
namespace Setting = Settings::Setting;

namespace {
	constexpr double DefaultTimeLimit = 10.9899997711182; // The default pre-song timer for Non-Stop Play.
}

std::string_view NonStopPlayTimerMod::Id() const {
	return "NonStopPlayTimer";
}

void NonStopPlayTimerMod::OnMenuTick(ModContext& c) {
	EnforceTimer(c);
}

void NonStopPlayTimerMod::OnSongTick(ModContext& c) {
	EnforceTimer(c);
}

// Continuously enforces the Non-Stop Play pre-song timer to the desired value (custom or default),
// re-correcting whenever the game's value drifts. Post-load ticks only, never during Loading.
void NonStopPlayTimerMod::EnforceTimer(ModContext& c) {
	const double desired = c.IsOn(Setting::UseCustomNSPTimer)
		? c.Int(Setting::CustomNSPTimeLimit) / 1000.0
		: DefaultTimeLimit;

	const double current = SongTimer::GetNonStopPlayTimer();

	const double eps = std::numeric_limits<double>::epsilon() * std::max(1.0, std::max(std::abs(desired), std::abs(current))) * 4;
	if (std::abs(current - desired) > eps) {
		LOG_INFO("Updating NSP timer...");
		SongTimer::SetNonStopPlayTimer(desired);
	}
}

static Framework::ModRegistrar<NonStopPlayTimerMod> _nonStopPlayTimerReg;
