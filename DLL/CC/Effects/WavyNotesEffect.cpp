#include "../../stdafx.h"
#include "WavyNotesEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	Enums::EffectStatus WavyNotesEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("WavyNotesEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}


	Enums::EffectStatus WavyNotesEffect::Start(const Structs::Request& request)
	{
		LOG_INFO("WavyNotesEffect::Start()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;
		
		SetDuration(request);
		running = true;
		wavyNotesEnabled = true;

		return Enums::EffectStatus::Success;
	}

	Enums::EffectStatus WavyNotesEffect::Stop()
	{
		LOG_INFO("WavyNotesEffect::Stop()" << std::endl);

		wavyNotesEnabled = false;
		running = false;

		return Enums::EffectStatus::Success;
	}
}