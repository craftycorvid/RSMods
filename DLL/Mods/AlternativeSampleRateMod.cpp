#include "../stdafx.h"
#include "AlternativeSampleRateMod.hpp"

using Framework::ModContext;
using Framework::GamePhase;
namespace Setting = Settings::Setting;

bool AlternativeSampleRateMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::AltOutputSampleRate);
}

// Patch the markers only during the loading phase, before the engine finishes coming up. The buffer
// checks keep it idempotent and avoid clobbering the values (5 / 2) the engine sets for itself.
void AlternativeSampleRateMod::OnTick(ModContext& c) {
	if (c.phase != GamePhase::Loading || c.Int(Setting::AlternativeOutputSampleRate) == 48000) {
		return;
	}

	if (*(int*)Offsets::ptr_sampleRateBuffer.Get() != 5 &&
		*(int*)Offsets::ptr_sampleRateBuffer.Get() != 2) {
		*(int*)Offsets::ptr_sampleRateSize.Get() = 2;
		*(int*)Offsets::ptr_sampleRateBuffer.Get() = 128;
	}
}

static Framework::ModRegistrar<AlternativeSampleRateMod> _altSampleRateReg;
