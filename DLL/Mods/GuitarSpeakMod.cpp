#include "../stdafx.h"
#include "GuitarSpeakMod.hpp"
#include "GuitarSpeak.hpp"

using Framework::ModContext;

std::string_view GuitarSpeakMod::Id() const {
	return "GuitarSpeak";
}

void GuitarSpeakMod::OnMenuTick(ModContext& c) {
	if (!guitarSpeakPresent && c.IsOn("GuitarSpeak")) {
		guitarSpeakPresent = true;
		if (!GuitarSpeak::RunGuitarSpeak()) { // If we are in a menu where we don't want to read bad values
			guitarSpeakPresent = false;
		}
	}
}

// Guitar Speak restarts each time the player returns to a menu, so clear the latch while in a song.
void GuitarSpeakMod::OnSongTick(ModContext&) {
	guitarSpeakPresent = false;
}

static Framework::ModRegistrar<GuitarSpeakMod> _guitarSpeakReg;
