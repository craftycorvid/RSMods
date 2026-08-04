#include "../stdafx.h"
#include "ScreenShotScoresMod.hpp"
#include "../Keyboard.hpp"

using Framework::ModContext;

std::string_view ScreenShotScoresMod::Id() const {
	return "ScreenShotScores";
}

void ScreenShotScoresMod::OnMenuTick(ModContext& c) {
	if (c.IsOn("ScreenShotScores") && GameState::Menus::IsInScoreMenus()) {
		Keyboard::TakeScreenshot();
	}
	else {
		Keyboard::takenScreenshotOfThisScreen = false;
	}
}

static Framework::ModRegistrar<ScreenShotScoresMod> _screenShotScoresReg;
