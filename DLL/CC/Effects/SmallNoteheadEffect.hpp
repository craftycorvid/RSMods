#pragma once

namespace CrowdControl::Effects {
	class SmallNoteheadEffect : public CCEffect
	{
	public:
		explicit SmallNoteheadEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "transparentnotes", "bignoteheads", "removenotes" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		Enums::EffectStatus Stop() override;

	private:
		static void SetNoteHeadScale(float scale);
	};
}
