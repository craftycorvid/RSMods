#include "../../stdafx.h"
#include "RainbowNotesEffect.hpp"

namespace CrowdControl::Effects {
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus RainbowNotesEffect::Test(const Request& request)
	{
		LOG_INFO("RainbowNotesEffect::Test()" << std::endl);

		if (!CanStart())
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Makes note heads continuously shift their colors 
	/// All note head changing is handled in ERMode, so it just toggles the switch in there
	/// Does not affect the strings!
	/// </summary>
	/// <returns> EffectStatus::Retry if we aren't currently in a song or the same effect is running already, or EffectStatus::Success if we are in a song</returns>
	EffectStatus RainbowNotesEffect::Start(const Request& request)
	{
		LOG_INFO("RainbowNotesEffect::Start()" << std::endl);

		if (!CanStart())
			return EffectStatus::Retry;

		ERMode::ToggleRainbowNotes();

		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus RainbowNotesEffect::Stop()
	{
		LOG_INFO("RainbowNotesEffect::Stop()" << std::endl);

		running = false;
		ERMode::ToggleRainbowNotes();

		return EffectStatus::Success;
	}
}