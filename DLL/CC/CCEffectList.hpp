#pragma once

#include "CCEffect.hpp"
#include "CCStructs.hpp"

namespace CrowdControl::EffectList {
	std::map<std::string, CrowdControl::Effects::CCEffect*>& GetAllEffects();

	bool AreIncompatibleEffectsEnabled(std::vector<std::string> incompatibleEffects);
	bool IsEffectEnabled(std::string effectName);
}
