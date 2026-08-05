#include "../../stdafx.h"
#include "TransparentNotesEffect.hpp"
#include "../../Framework/Framework.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Changes textures for noteheads to a nonexistent (effectively transparent) texture
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus TransparentNotesEffect::Test(const Request& request)
	{
		LOG_INFO("TransparentNotesEffect::Test()" << std::endl);

		if (!CanStart())
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Make the notes appear transparent.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus TransparentNotesEffect::Start(const Request& request)
	{
		LOG_INFO("TransparentNotesEffect::Start()" << std::endl);

		if (!CanStart())
			return EffectStatus::Retry;

		Framework::Registry().EnqueueSettingsUpdate([] {
			Settings::UpdateTwitchSetting("TransparentNotes", "on");
		});
		
		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus TransparentNotesEffect::Stop()
	{
		LOG_INFO("TransparentNotesEffect::Stop()" << std::endl);

		Framework::Registry().EnqueueSettingsUpdate([] {
			Settings::UpdateTwitchSetting("TransparentNotes", "off");
		});
		running = false;

		return EffectStatus::Success;
	}
}
