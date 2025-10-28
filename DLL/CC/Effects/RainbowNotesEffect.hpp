#pragma once
#include "../../Mods/ExtendedRangeMode.hpp"

namespace CrowdControl::Effects {
	class RainbowNotesEffect : public CCEffect
	{
	public:
		explicit RainbowNotesEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "removenotes", "transparentnotes", "solidrandom",  "solidcustom", "solidcustomrgb" };
		}
		
		/**
		 * \brief Can this effect start? By default checks that a song is being played, no incompatible effects are running, and this effect is not running
		 * \return True when this effect can start, false otherwise
		 */
		bool CanStart() override
		{
			return GameState::IsInSong() && !ERMode::IsRainbowNotesEnabled() && !AreIncompatibleEffectsRunning() && !running;
		}

		EffectStatus Test(const Request& request) override;
		EffectStatus Start(const Request& request) override;
		EffectStatus Stop() override;
	};
}
