#pragma once

#include "CCEffect.hpp"
#include "CCStructs.hpp"

namespace CrowdControl::EffectList {
	std::map<std::string, CrowdControl::Effects::CCEffect*>& GetAllEffects();

	bool AreIncompatibleEffectsEnabled(const std::vector<std::string>& incompatibleEffects);
	bool IsEffectEnabled(const std::string& effectName);
}
