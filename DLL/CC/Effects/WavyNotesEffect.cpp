#include "../../stdafx.h"
#include "WavyNotesEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectStatus WavyNotesEffect::Test(const Request& request)
	{
		LOG_INFO("WavyNotesEffect::Test()" << std::endl);

		if (!CanStart())
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}


	EffectStatus WavyNotesEffect::Start(const Request& request)
	{
		LOG_INFO("WavyNotesEffect::Start()" << std::endl);

		if (!CanStart())
			return EffectStatus::Retry;
		
		SetDuration(request);
		running = true;
		wavyNotesEnabled = true;

		return EffectStatus::Success;
	}

	EffectStatus WavyNotesEffect::Stop()
	{
		LOG_INFO("WavyNotesEffect::Stop()" << std::endl);

		wavyNotesEnabled = false;
		running = false;

		return EffectStatus::Success;
	}
}