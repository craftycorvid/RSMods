#pragma once

namespace CrowdControl::Effects {
	class FYourFCEffect : public CCEffect
	{
	public:
		FYourFCEffect() = default;

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		Enums::EffectStatus Stop() override;
	};
}
