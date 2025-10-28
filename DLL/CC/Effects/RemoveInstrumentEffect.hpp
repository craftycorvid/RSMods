#pragma once

namespace CrowdControl::Effects {
	class RemoveInstrumentEffect : public CCEffect
	{
	public:
		explicit RemoveInstrumentEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "invertedstrings", "rainbowstrings" };
		}

		EffectStatus Test(const Request& request) override;
		EffectStatus Start(const Request& request) override;
		EffectStatus Stop() override;

	private:
		static void SetInstrumentScale(float scale);
	};
}