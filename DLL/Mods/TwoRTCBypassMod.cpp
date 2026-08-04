#include "../stdafx.h"
#include "TwoRTCBypassMod.hpp"

using Framework::ModContext;

std::string_view TwoRTCBypassMod::Id() const {
	return "BypassTwoRTCMessageBox";
}

void TwoRTCBypassMod::OnMenuTick(ModContext& c) {
	SyncPatch(c);
}

void TwoRTCBypassMod::OnSongTick(ModContext& c) {
	SyncPatch(c);
}

// Keeps the in-memory patch in sync with the setting, both applying and reverting as it toggles.
// The memory-state comparisons make this idempotent. Post-load ticks only: it patches game memory,
// so it must never run during Loading.
void TwoRTCBypassMod::SyncPatch(ModContext& c) {
	if (c.IsOff("BypassTwoRTCMessageBox") && *(char*)Offsets::ptr_twoRTCBypass.Get() == Offsets::ptr_twoRTCBypass_patch_call[0]) {
		MemUtil::PatchAdr((LPVOID)Offsets::ptr_twoRTCBypass.Get(), (LPVOID)Offsets::ptr_twoRTCBypass_original, 6);
	}
	else if (c.IsOn("BypassTwoRTCMessageBox") && *(char*)Offsets::ptr_twoRTCBypass.Get() == Offsets::ptr_twoRTCBypass_original[0]) {
		QualityOfLife::PatchTwoRTC();
	}
}

static Framework::ModRegistrar<TwoRTCBypassMod> _twoRTCBypassReg;
