#pragma once

namespace CrowdControl::Effects {
	class KillGuitarVolumeEffect : public CCEffect
	{
	public:
		explicit KillGuitarVolumeEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		EffectStatus Test(const Request& request) override;
		EffectStatus Start(const Request& request) override;
		void Run() override;
		EffectStatus Stop() override;

	private:
		float oldVolume = 100.0f;
		bool ending = false;
	};
}
#pragma once
