#include "../../stdafx.h"
#include "BigNoteheadEffect.hpp"

namespace CrowdControl::Effects {
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus BigNoteheadEffect::Test(const Request& request)
	{
		LOG_INFO("BigNoteheadEffect::Test()" << std::endl);

		if (!CanStart())
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Start the mod by making noteheads 2.5x the size.
	/// </summary>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus BigNoteheadEffect::Start(const Request& request)
	{
		LOG_INFO("BigNoteheadEffect::Start()" << std::endl);

		if (!CanStart())
			return EffectStatus::Retry;

		SetNoteHeadScale(2.5);
	
		SetDuration(request);
		running = true;
		
		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus BigNoteheadEffect::Stop()
	{
		LOG_INFO("BigNoteheadEffect::Stop()" << std::endl);

		SetNoteHeadScale(1);
		running = false;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Sets the scale only for objects which are of note mesh type
	/// </summary>
	void BigNoteheadEffect::SetNoteHeadScale(float scale) {
		LOG_INFO("BigNoteheadEffect::SetNoteHeadScale(" << scale << ")" << std::endl);

		ObjectUtil::SetObjectScales({
			{"MeshNoteSingleLH", scale},
			{"MeshNoteSingleRH", scale}
		});
	}
}
