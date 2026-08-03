#include "../stdafx.h"
#include "RemoveHeadstockMod.hpp"

using Framework::ModContext;
using Settings::When;

std::string_view RemoveHeadstockMod::Id() const {
	return "RemoveHeadstock";
}

bool RemoveHeadstockMod::IsEnabled(const ModContext& c) const {
	return c.IsOn("RemoveHeadstockEnabled");
}

void RemoveHeadstockMod::OnSongTick(ModContext& c) {
	if (c.WhenSetting("RemoveHeadstockWhen") == When::Song) {
		D3DHooks::RemoveHeadstockInThisMenu = true;
	}
	ApplyStartup(c);
}

void RemoveHeadstockMod::OnMenuTick(ModContext& c) {
	// Song mode hides the headstock only while playing, so restore it when back in menus.
	if (c.WhenSetting("RemoveHeadstockWhen") == When::Song) {
		D3DHooks::RemoveHeadstockInThisMenu = false;
	}
	ApplyStartup(c);
}

void RemoveHeadstockMod::ApplyStartup(ModContext& c) {
	if (c.WhenSetting("RemoveHeadstockWhen") == When::Startup) {
		D3DHooks::RemoveHeadstockInThisMenu = true;
	}
}

static Framework::ModRegistrar<RemoveHeadstockMod> _removeHeadstockReg;
