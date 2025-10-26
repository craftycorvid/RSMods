#include "../../stdafx.h"
#include "WavyNotesEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectStatus WavyNotesEffect::Test(Request request)
	{
		LOG_INFO("WavyNotesEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::GetAllEffects()))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}


	EffectStatus WavyNotesEffect::Start(Request request)
	{
		LOG_INFO("WavyNotesEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::GetAllEffects()))
			return EffectStatus::Retry;
		
		SetDuration(request);
		running = true;
		wavy_notes_enabled = true;

		return EffectStatus::Success;
	}

	EffectStatus WavyNotesEffect::Stop()
	{
		LOG_INFO("WavyNotesEffect::Stop()" << std::endl);

		wavy_notes_enabled = false;
		running = false;

		return EffectStatus::Success;
	}
}