#include "../stdafx.h"
#include "RemoveHeadstockMod.hpp"

using Framework::ModContext;
using Settings::When;
namespace Setting = Settings::Setting;

bool RemoveHeadstockMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::RemoveHeadstockEnabled);
}

void RemoveHeadstockMod::OnSongTick(ModContext& c) {
	if (c.When(Setting::RemoveHeadstockWhen) == When::Song) {
		D3DHooks::RemoveHeadstockInThisMenu = true;
	}
	ApplyStartup(c);
}

void RemoveHeadstockMod::OnMenuTick(ModContext& c) {
	// Song mode hides the headstock only while playing, so restore it when back in menus.
	if (c.When(Setting::RemoveHeadstockWhen) == When::Song) {
		D3DHooks::RemoveHeadstockInThisMenu = false;
	}
	ApplyStartup(c);
}

void RemoveHeadstockMod::ApplyStartup(ModContext& c) {
	if (c.When(Setting::RemoveHeadstockWhen) == When::Startup) {
		D3DHooks::RemoveHeadstockInThisMenu = true;
	}
}

static Framework::ModRegistrar<RemoveHeadstockMod> _removeHeadstockReg;
