#include "../stdafx.h"
#include "LoftMod.hpp"
#include "Loft.hpp"

using Framework::ModContext;
using Settings::When;

std::string_view LoftMod::Id() const {
	return "Loft";
}

bool LoftMod::IsEnabled(const ModContext& c) const {
	return c.IsOn("ToggleLoftEnabled");
}

void LoftMod::OnDisabled(ModContext&) {
	if (loftOff) {
		Loft::ToggleLoft();
		loftOff = false;
	}
	D3DHooks::GreenScreenWall = false;
}

void LoftMod::OnSongTick(ModContext& c) {
	if (c.WhenSetting("ToggleLoftWhen") == When::Song) {
		if (!loftOff) {
			Loft::ToggleLoft();
		}
		loftOff = true;
	}
	ApplyAlwaysOn(c);
}

void LoftMod::OnMenuTick(ModContext& c) {
	if (c.WhenSetting("ToggleLoftWhen") == When::Song) {
		if (loftOff) {
			Loft::ToggleLoft();
			loftOff = false;
		}
		
		if (!GameState::LessonMode) D3DHooks::GreenScreenWall = false;
	}
	ApplyAlwaysOn(c);
}

// Runs after the phase policy so lesson mode takes precedence.
void LoftMod::ApplyAlwaysOn(ModContext& c) {
	const When when = c.WhenSetting("ToggleLoftWhen");
	if (when == When::Manual) {
		if (loftOff) {
			Loft::ToggleLoft();
			loftOff = false;
		}
		D3DHooks::GreenScreenWall = false;
		return;
	}

	// Lesson videos require the loft; GreenScreenWall preserves the no-loft appearance.
	if (GameState::LessonMode && when != When::Manual) {
		if (loftOff)
			Loft::ToggleLoft();
		loftOff = false;
		D3DHooks::GreenScreenWall = true;
	}

	if (!loftOff && !GameState::LessonMode && when == When::Startup) {
		Loft::ToggleLoft();
		loftOff = true;
		D3DHooks::GreenScreenWall = false;
	}
}

static Framework::ModRegistrar<LoftMod> _loftReg;
