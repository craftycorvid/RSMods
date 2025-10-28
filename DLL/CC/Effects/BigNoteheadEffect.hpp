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

		EffectStatus Test(const Request& request) override;
		EffectStatus Start(const Request& request) override;
		EffectStatus Stop() override;

	private:
		static void SetNoteHeadScale(float scale);
	};
}
