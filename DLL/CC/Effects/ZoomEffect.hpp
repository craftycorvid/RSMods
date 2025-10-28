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

		EffectStatus Test(const Request& request) override;
		EffectStatus Start(const Request& request) override;
		EffectStatus Stop() override;
	};
}
