#include "../stdafx.h"
#include "RemoveLyricsMod.hpp"

using Framework::ModContext;
using Settings::When;

std::string_view RemoveLyricsMod::Id() const {
	return "RemoveLyrics";
}

bool RemoveLyricsMod::IsEnabled(const ModContext& c) const {
	return c.IsOn("RemoveLyrics");
}

void RemoveLyricsMod::OnSongTick(ModContext& c) {
	ApplyStartup(c);
}

void RemoveLyricsMod::OnMenuTick(ModContext& c) {
	ApplyStartup(c);
}

// "Startup" mode latches lyric removal on once loaded; the manual keybinding and the
// render-time setting check own every other case, so this is the mod's only policy.
void RemoveLyricsMod::ApplyStartup(ModContext& c) {
	if (!D3DHooks::RemoveLyrics && c.WhenSetting("RemoveLyricsWhen") == When::Startup) {
		D3DHooks::RemoveLyrics = true;
	}
}

static Framework::ModRegistrar<RemoveLyricsMod> _removeLyricsReg;
