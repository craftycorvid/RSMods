#include "../../stdafx.h"
#include "ChangeToToneSlot.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus ChangeToToneSlot::Test(const Request& request)
	{
		LOG_INFO("ChangeToToneSlot::Test()" << std::endl);

		if (!CanStart())
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}


	/// <summary>
	/// Sends a keystroke to the game for the current tone slot (presses number 1 for first tone slot, number 2 for second, etc.)
	/// </summary>
	/// <returns> EffectStatus::Retry if we aren't currently in a song, or EffectStatus::Success if we are</returns>
	EffectStatus ChangeToToneSlot::Start(const Request& request)
	{
		LOG_INFO("ChangeToToneSlot::Start()" << std::endl);

		if (!CanStart())
			return EffectStatus::Retry;

		Util::SendKey(Settings::GetVKCodeForString(std::to_string(slot)));
	
		LOG_INFO("Changing tone to slot " << slot << std::endl);

		return EffectStatus::Success;
	}

	/// <summary>
	/// Mod cannot be stopped, since it has an instant effect.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus ChangeToToneSlot::Stop() { return EffectStatus::Success; }
}