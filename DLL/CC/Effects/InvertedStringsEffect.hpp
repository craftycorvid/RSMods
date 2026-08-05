#pragma once

namespace CrowdControl::Effects {
	class InvertedStringsEffect : public CCEffect
	{
	public:
		explicit InvertedStringsEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "removeinstrument" };
		}

		Enums::EffectStatus Test(const Structs::Request& request);
		Enums::EffectStatus Start(const Structs::Request& request);
		Enums::EffectStatus Stop();
	};
}