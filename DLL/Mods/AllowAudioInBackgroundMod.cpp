#include "../stdafx.h"
#include "AllowAudioInBackgroundMod.hpp"
#include "VolumeControl.hpp"

using Framework::ModContext;
namespace Setting = Settings::Setting;

void AllowAudioInBackgroundMod::OnMenuTick(ModContext& c) {
	SyncState(c);
}

void AllowAudioInBackgroundMod::OnSongTick(ModContext& c) {
	SyncState(c);
}

void AllowAudioInBackgroundMod::SyncState(ModContext& c) {
	if (c.IsOn(Setting::AllowAudioInBackground) && !VolumeControl::allowedAltTabbingWithAudio) {
		VolumeControl::AllowAltTabbingWithAudio();
	}
	else if (c.IsOff(Setting::AllowAudioInBackground) && VolumeControl::allowedAltTabbingWithAudio) {
		VolumeControl::DisableAltTabbingWithAudio();
	}
}

static Framework::ModRegistrar<AllowAudioInBackgroundMod> _allowAudioInBackgroundReg;
