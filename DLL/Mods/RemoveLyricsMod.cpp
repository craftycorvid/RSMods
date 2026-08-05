#include "../stdafx.h"
#include "RemoveLyricsMod.hpp"

using Framework::ModContext;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Settings::When;

std::string_view RemoveLyricsMod::Id() const {
	return "RemoveLyrics";
}

bool RemoveLyricsMod::IsEnabled(const ModContext& c) const {
	return c.IsOn("RemoveLyrics");
}

void RemoveLyricsMod::OnInitialize(ModContext& c) {
	c.Commands().BindSetting(
		"RemoveLyricsKey",
		KeyEdge::Up,
		Availability::Initialized,
		[](ModContext&, const KeyEvent&) {
			D3DHooks::RemoveLyrics = !D3DHooks::RemoveLyrics;
		},
		[](const ModContext& context, const KeyEvent&) {
			return context.WhenSetting("RemoveLyricsWhen") == When::Manual;
		},
		"Remove Lyrics");
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
