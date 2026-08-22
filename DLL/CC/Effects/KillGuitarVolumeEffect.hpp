#pragma once

namespace CrowdControl::Effects {
	class KillGuitarVolumeEffect : public CCEffect
	{
	public:
		explicit KillGuitarVolumeEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		void Run() override;
		Enums::EffectStatus Stop() override;

	private:
		float oldVolume = 100.0f;
		bool ending = false;
	};
}
