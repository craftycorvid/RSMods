#include "../stdafx.h"
#include "SongPreviewsMod.hpp"
#include "VolumeControl.hpp"

using Framework::ModContext;

std::string_view SongPreviewsMod::Id() const {
	return "SongPreviews";
}

void SongPreviewsMod::OnMenuTick(ModContext& c) {
	SyncState(c);
}

// Keeps song-preview audio in sync with the setting, both muting and restoring as it toggles.
// The VolumeControl::disabledSongPreviewAudio flag makes this idempotent.
void SongPreviewsMod::SyncState(ModContext& c) {
	if (c.IsOn(Id())) {
		if (!VolumeControl::disabledSongPreviewAudio) {
			VolumeControl::DisableSongPreviewAudio();
		}
	}
	else if (VolumeControl::disabledSongPreviewAudio) { // User originally wanted song previews off, but now wants them on.
		VolumeControl::EnableSongPreviewAudio();
	}
}

static Framework::ModRegistrar<SongPreviewsMod> _songPreviewsReg;
