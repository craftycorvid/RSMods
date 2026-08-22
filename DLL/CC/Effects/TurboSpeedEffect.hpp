#pragma once
#include "../../Mods/RiffRepeater.hpp"

namespace CrowdControl::Effects {
	class TurboSpeedEffect : public CCEffect
	{
	public:
		explicit TurboSpeedEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		Enums::EffectStatus Stop() override;
	};
}
