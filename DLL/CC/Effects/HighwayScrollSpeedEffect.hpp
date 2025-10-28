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

		EffectStatus Test(const Request& request);
		EffectStatus Start(const Request& request);
		EffectStatus Stop();

	private:
		static void WriteScrollSpeedMultiplier(double val);
	};
}

#pragma once
