#pragma once
#include "../../Mods/Loft.hpp"

namespace CrowdControl::Effects {
	class DrunkModeEffect : public CCEffect
	{
	public:
		explicit DrunkModeEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		EffectStatus Test(const Request& request) override;
		EffectStatus Start(const Request& request) override;
		EffectStatus Stop() override;
	};
}
