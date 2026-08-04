#include "../stdafx.h"
#include "VolumeDisplayMod.hpp"

using Framework::ModContext;

std::string_view VolumeDisplayMod::Id() const {
	return "VolumeControlEnabled";
}

void VolumeDisplayMod::OnMenuTick(ModContext& c) {
	SyncDisplay(c);
}

void VolumeDisplayMod::OnSongTick(ModContext& c) {
	SyncDisplay(c);
}

// The volume overlay is raised by the volume keybindings; hide it again once it has been up for 3s.
void VolumeDisplayMod::SyncDisplay(ModContext& c) {
	if (c.IsOn("VolumeControlEnabled") && MoreThanThreeSecondsPassed()) {
		GameOverlay::displayCurrentVolume = false;
	}
}

bool VolumeDisplayMod::MoreThanThreeSecondsPassed() const {
	const auto currentTime = std::chrono::steady_clock::now();
	return currentTime - GameOverlay::displayVolumeStartTime > std::chrono::seconds(3);
}

static Framework::ModRegistrar<VolumeDisplayMod> _volumeDisplayReg;
