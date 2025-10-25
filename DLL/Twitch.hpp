#pragma once

#include "Mods/Loft.hpp"

namespace Twitch {
	inline std::vector<std::string> effectQueue;
	inline std::vector<std::string> enabledEffects;

	bool HandleMessage(std::string const& currMsg, std::string const& type);
	void HandleEffect(std::string const& currEffectMsg);
	void ParseEffectQueue();
	bool IsCurrentEffectAlreadyAppliedOrNotInSong(const std::string& effectName);
	void DisableEffect(const std::string& effectName);
}