#pragma once
#include "../../Mods/ExtendedRangeMode.hpp"

namespace CrowdControl::Effects {
	// Changes current note heads to a custom generated texture
	class SolidNotesRandomEffect : public CCEffect
	{
	public:
		explicit SolidNotesRandomEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "removenotes", "transparentnotes", "solidcustom", "solidcustomrgb", "rainbownotes" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		Enums::EffectStatus Stop() override;

		/**
		 * \brief Can this effect start? By default checks that a song is being played, no incompatible effects are running, and this effect is not running
		 * \return True when this effect can start, false otherwise
		 */
		bool CanStart() override
		{
			return ERMode::ColorsSaved && GameState::IsInSong() && !AreIncompatibleEffectsRunning() && !running;
		}

	};

	class SolidNotesCustomEffect : public CCEffect
	{
	public:
		explicit SolidNotesCustomEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "removenotes", "transparentnotes", "solidrandom", "solidcustomrgb", "rainbownotes" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		Enums::EffectStatus Stop() override;
	};

	class SolidNotesCustomRGBEffect : public CCEffect
	{
	public:
		explicit SolidNotesCustomRGBEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "removenotes", "transparentnotes", "solidcustom", "solidrandom", "rainbownotes" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		Enums::EffectStatus Stop() override;
	};
}
