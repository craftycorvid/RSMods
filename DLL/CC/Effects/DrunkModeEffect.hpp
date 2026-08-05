#pragma once
#include "../../Mods/Loft.hpp"

namespace CrowdControl::Effects {
	class DrunkModeEffect : public CCEffect
	{
	public:
		explicit DrunkModeEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		Enums::EffectStatus Stop() override;
	};
}
