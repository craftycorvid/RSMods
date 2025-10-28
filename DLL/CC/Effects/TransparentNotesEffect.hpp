#pragma once

namespace CrowdControl::Effects {
	class TransparentNotesEffect : public CCEffect
	{
	public:
		explicit TransparentNotesEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "solidcustom", "solidrandom", "solidcustomrgb", "bignoteheads", "smallnoteheads", "removenotes" };
		}

		EffectStatus Test(const Request& request) override;
		EffectStatus Start(const Request& request) override;
		EffectStatus Stop() override;
	};
}
