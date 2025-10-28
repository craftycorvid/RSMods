#pragma once
#include "../../Mods/RiffRepeater.hpp"

namespace CrowdControl::Effects {
	class TurboSpeedEffect : public CCEffect
	{
	public:
		explicit TurboSpeedEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		EffectStatus Test(const Request& request) override;
		EffectStatus Start(const Request& request) override;
		EffectStatus Stop() override;
	};
}
