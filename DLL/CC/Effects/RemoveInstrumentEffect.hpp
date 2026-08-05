#pragma once

namespace CrowdControl::Effects {
	class RemoveInstrumentEffect : public CCEffect
	{
	public:
		explicit RemoveInstrumentEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "invertedstrings", "rainbowstrings" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		Enums::EffectStatus Stop() override;

	private:
		static void SetInstrumentScale(float scale);
	};
}