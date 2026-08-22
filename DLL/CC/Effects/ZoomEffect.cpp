#include "../../stdafx.h"
#include "ZoomEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	Enums::EffectStatus ZoomEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("ZoomEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}


	Enums::EffectStatus ZoomEffect::Start(const Structs::Request& request)
	{
		LOG_INFO("ZoomEffect::Start()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		auto rootObject = ObjectUtil::GetRootObject();

		if (rootObject)
		{
			rootObject->scale = factor;
		}

		
		SetDuration(request);
		running = true;

		return Enums::EffectStatus::Success;
	}

	Enums::EffectStatus ZoomEffect::Stop()
	{
		LOG_INFO("ZoomEffect::Stop()" << std::endl);

		auto rootObject = ObjectUtil::GetRootObject();

		if (rootObject)
		{
			rootObject->scale = 1.0f;
		}

		running = false;

		return Enums::EffectStatus::Success;
	}
}