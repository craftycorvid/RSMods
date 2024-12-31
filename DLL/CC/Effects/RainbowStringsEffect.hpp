#pragma once
#include "../../Mods/ExtendedRangeMode.hpp"

namespace CrowdControl::Effects {
	class RainbowStringsEffect : public CCEffect
	{
	public:
		RainbowStringsEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "removeinstrument" };
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;

		/**
		 * \brief Can this effect start? By default checks that a song is being played, no incompatible effects are running, and this effect is not running
		 * \return True when this effect can start, false otherwise
		 */
		bool CanStart(std::map<std::string, CCEffect*>* AllEffects) override
		{
			return !ERMode::IsRainbowEnabled() && GameState::IsInSong() && !AreIncompatibleEffectsRunning(AllEffects) && !running;
		}
	};

}
