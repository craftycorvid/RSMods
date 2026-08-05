#pragma once

namespace CrowdControl::Effects {
	class ChangeToToneSlot : public CCEffect
	{
	public:
		int slot;

		explicit ChangeToToneSlot(int _slot) : slot(_slot){
			incompatibleEffects = { "shuffletones" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		Enums::EffectStatus Start(const Structs::Request& request) override;
		Enums::EffectStatus Stop() override;
	};
}