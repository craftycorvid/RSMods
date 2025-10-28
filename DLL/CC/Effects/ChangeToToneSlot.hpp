#pragma once

namespace CrowdControl::Effects {
	class ChangeToToneSlot : public CCEffect
	{
	public:
		int slot;

		explicit ChangeToToneSlot(int _slot) : slot(_slot){
			incompatibleEffects = { "shuffletones" };
		}

		EffectStatus Test(const Request& request) override;
		EffectStatus Start(const Request& request) override;
		EffectStatus Stop() override;
	};
}