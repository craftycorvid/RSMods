#pragma once

namespace CrowdControl::Effects {
	class ShuffleTonesEffect : public CCEffect
	{
	public:
		explicit ShuffleTonesEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		uint32_t tickIntervalMilliseconds = 2000;
		std::chrono::steady_clock::time_point nextTickTime;

		EffectStatus Test(const Request& request) override;
		EffectStatus Start(const Request& request) override;
		void Run() override;
		EffectStatus Stop() override;
	};
}