#pragma once

namespace CrowdControl::Effects {
	class SmallNoteheadEffect : public CCEffect
	{
	public:
		explicit SmallNoteheadEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "transparentnotes", "bignoteheads", "removenotes" };
		}

		EffectStatus Test(const Request& request) override;
		EffectStatus Start(const Request& request) override;
		EffectStatus Stop() override;

	private:
		static void SetNoteHeadScale(float scale);
	};
}
