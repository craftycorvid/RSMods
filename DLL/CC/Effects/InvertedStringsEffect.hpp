#pragma once

namespace CrowdControl::Effects {
	class InvertedStringsEffect : public CCEffect
	{
	public:
		explicit InvertedStringsEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "removeinstrument" };
		}

		EffectStatus Test(const Request& request);
		EffectStatus Start(const Request& request);
		EffectStatus Stop();
	};
}