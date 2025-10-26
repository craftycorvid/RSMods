#include "../../stdafx.h"
#include "ZoomEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectStatus ZoomEffect::Test(Request request)
	{
		LOG_INFO("ZoomEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::GetAllEffects()))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}


	EffectStatus ZoomEffect::Start(Request request)
	{
		LOG_INFO("ZoomEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::GetAllEffects()))
			return EffectStatus::Retry;

		auto rootObject = ObjectUtil::GetRootObject();

		if (rootObject)
		{
			rootObject->scale = factor;
		}

		
		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	EffectStatus ZoomEffect::Stop()
	{
		LOG_INFO("ZoomEffect::Stop()" << std::endl);

		auto rootObject = ObjectUtil::GetRootObject();

		if (rootObject)
		{
			rootObject->scale = 1.0f;
		}

		running = false;

		return EffectStatus::Success;
	}
}