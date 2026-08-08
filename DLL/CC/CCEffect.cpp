#include "../stdafx.h"
#include "CCEffect.hpp"

namespace CrowdControl::Effects {
	void CCEffect::Run() {
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	void CCEffect::SetDuration(const Structs::Request& req) {
		if (req.duration)
			duration_ms = req.duration;

		// Is this ever used?
		for (const auto& el : req.parameters.items()) {
			if (el.value().contains("duration")) {
				el.value().at("duration").get_to(duration_ms);
				// Assuming this is in seconds, convert to ms
				duration_ms *= 1000;
				break;
			}
		}

		LOG_INFO("Set duration to " << duration_ms << "ms" << std::endl);

		endTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);
	}

	bool CCEffect::CanStart() {
		return GameState::IsInSong() && !AreIncompatibleEffectsRunning() && !running;
	}

	bool CCEffect::AreIncompatibleEffectsRunning() const {
		const auto& allEffects = CrowdControl::EffectList::GetAllEffects();
		return std::ranges::any_of(incompatibleEffects,
			[&allEffects](const std::string& effectName) {
				auto it = allEffects.find(effectName);
				return it != allEffects.end() && it->second->running;
			});
	}
}