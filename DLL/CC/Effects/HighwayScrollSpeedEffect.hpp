#pragma once

namespace CrowdControl::Effects {
	class HighwayScrollSpeedEffect : public CCEffect
	{
	public:
		double multiplier = 5.0;

		explicit HighwayScrollSpeedEffect(int64_t durationMilliseconds, double speedMultiplier) : multiplier(speedMultiplier)
		{
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "halfscrollspeed", "doublescrollspeed", "triplescrollspeed" };
		}

		Enums::EffectStatus Test(const Structs::Request& request);
		Enums::EffectStatus Start(const Structs::Request& request);
		Enums::EffectStatus Stop();

	private:
		static void WriteScrollSpeedMultiplier(double val);
	};
}
