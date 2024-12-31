#pragma once

namespace CrowdControl::Effects {
	class ChangeToToneSlot : public CCEffect
	{
	public:
		int slot;

		ChangeToToneSlot(int _slot) {
			slot = _slot;
			incompatibleEffects = { "shuffletones" };
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;
	};
}