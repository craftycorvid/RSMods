#include "../stdafx.h"
#include "AllowAudioInBackgroundMod.hpp"
#include "VolumeControl.hpp"

using Framework::ModContext;

std::string_view AllowAudioInBackgroundMod::Id() const {
	return "AllowAudioInBackground";
}

void AllowAudioInBackgroundMod::OnMenuTick(ModContext& c) {
	SyncState(c);
}

void AllowAudioInBackgroundMod::OnSongTick(ModContext& c) {
	SyncState(c);
}

void AllowAudioInBackgroundMod::SyncState(ModContext& c) {
	if (c.IsOn("AllowAudioInBackground") && !VolumeControl::allowedAltTabbingWithAudio) {
		VolumeControl::AllowAltTabbingWithAudio();
	}
	else if (c.IsOff("AllowAudioInBackground") && VolumeControl::allowedAltTabbingWithAudio) {
		VolumeControl::DisableAltTabbingWithAudio();
	}
}

static Framework::ModRegistrar<AllowAudioInBackgroundMod> _allowAudioInBackgroundReg;
