#pragma once

namespace CrowdControl::Effects {
	// Prevents the game from drawing note head meshes
	class RemoveNotesEffect : public CCEffect
	{
	public:
		explicit RemoveNotesEffect(unsigned int durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "transparentnotes", "bignoteheads", "smallnoteheads", "wavynotes"};
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		Enums::EffectStatus Stop() override;

	private:
		static void ScaleNotes(float scale);
	};
}
