#pragma once

namespace CrowdControl::Effects {
	// Scales notes in a song to unusually big size
	class BigNoteheadEffect : public CCEffect
	{
	public:
		explicit BigNoteheadEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "transparentnotes", "smallnoteheads", "removenotes" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		Enums::EffectStatus Stop() override;

	private:
		static void SetNoteHeadScale(float scale);
	};
}
