#pragma once

namespace CrowdControl::Effects {
	class FYourFCEffect : public CCEffect
	{
	public:
		FYourFCEffect() = default;

		EffectStatus Test(const Request& request) override;
		EffectStatus Start(const Request& request) override;
		EffectStatus Stop() override;
	};
}
