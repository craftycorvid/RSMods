#include "../stdafx.h"
#include "ScreenShotScoresMod.hpp"
#include "../Keyboard.hpp"

using Framework::ModContext;
namespace Setting = Settings::Setting;

void ScreenShotScoresMod::OnMenuTick(ModContext& c) {
	if (c.IsOn(Setting::ScreenShotScores) && GameState::Menus::IsInScoreMenus()) {
		Keyboard::TakeScreenshot();
	}
	else {
		Keyboard::takenScreenshotOfThisScreen = false;
	}
}

static Framework::ModRegistrar<ScreenShotScoresMod> _screenShotScoresReg;
