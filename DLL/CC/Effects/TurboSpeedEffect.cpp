#include "../../stdafx.h"
#include "TurboSpeedEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {

	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus TurboSpeedEffect::Test(Request request)
	{
		LOG_INFO("TurboSpeedEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::GetAllEffects()))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Set Riff Repeater speed to 200%.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus TurboSpeedEffect::Start(Request request)
	{
		LOG_INFO("TurboSpeedEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::GetAllEffects()))
			return EffectStatus::Retry;
		
		RiffRepeater::SetSpeed(200.f, true);
		RiffRepeater::EnableTimeStretch();

		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus TurboSpeedEffect::Stop()
	{
		LOG_INFO("TurboSpeedEffect::Stop()" << std::endl);

		RiffRepeater::SetSpeed(100.f);
		RiffRepeater::DisableTimeStretch();

		running = false;

		return EffectStatus::Success;
	}
}