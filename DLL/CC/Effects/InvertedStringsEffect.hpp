#pragma once

namespace CrowdControl::Effects {
	class InvertedStringsEffect : public CCEffect
	{
	public:
		InvertedStringsEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "removeinstrument" };
		}

		EffectStatus Test(Request request);
		EffectStatus Start(Request request);
		EffectStatus Stop();
	};
}