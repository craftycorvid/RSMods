#pragma once

namespace CrowdControl::Effects {
	class TransparentNotesEffect : public CCEffect
	{
	public:
		explicit TransparentNotesEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "solidcustom", "solidrandom", "solidcustomrgb", "bignoteheads", "smallnoteheads", "removenotes" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		Enums::EffectStatus Stop() override;
	};
}
