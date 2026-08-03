#include "../stdafx.h"
#include "RemoveSkylineMod.hpp"

using Framework::ModContext;
using Settings::When;

std::string_view RemoveSkylineMod::Id() const {
	return "RemoveSkyline";
}

bool RemoveSkylineMod::IsEnabled(const ModContext& c) const {
	return c.IsOn("RemoveSkylineEnabled");
}

void RemoveSkylineMod::OnDisabled(ModContext&) {
	D3DHooks::toggleSkyline = false;
	D3DHooks::SkylineOff = false;
	D3DHooks::DrawSkylineInMenu = false;
}

void RemoveSkylineMod::OnSongTick(ModContext& c) {
	if (c.WhenSetting("ToggleSkylineWhen") == When::Song) {
		if (!D3DHooks::SkylineOff) {
			D3DHooks::toggleSkyline = true;
		}
		
		D3DHooks::DrawSkylineInMenu = false;
	}

	ApplyStartup(c);
}

void RemoveSkylineMod::OnMenuTick(ModContext& c) {
	// Coming back from a song with the skyline off: request the toggle and let it draw in the menu again.
	if (D3DHooks::SkylineOff && c.WhenSetting("ToggleSkylineWhen") == When::Song) {
		D3DHooks::toggleSkyline = true;
		D3DHooks::DrawSkylineInMenu = true;
	}

	ApplyStartup(c);
}

void RemoveSkylineMod::ApplyStartup(ModContext& c) {
	if (!D3DHooks::SkylineOff && c.WhenSetting("ToggleSkylineWhen") == When::Startup) {
		D3DHooks::toggleSkyline = true;
	}
}

static Framework::ModRegistrar<RemoveSkylineMod> _removeSkylineReg;
