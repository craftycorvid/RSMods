#pragma once

namespace CrowdControl::Effects {
	class ZoomEffect : public CCEffect
	{
	public:
		float factor = 1.0;

		explicit ZoomEffect(int64_t durationMilliseconds, float zoomFactor) : factor(zoomFactor)
		{
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "zoomin", "zoomout" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		Enums::EffectStatus Stop() override;
	};
}
