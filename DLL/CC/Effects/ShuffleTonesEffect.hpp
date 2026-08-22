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

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		void Run() override;
		Enums::EffectStatus Stop() override;
	};
}