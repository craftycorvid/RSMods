#include <iomanip>

#include "CommandCollisionDiagnostics.hpp"

#include <algorithm>
#include <map>
#include <tuple>
#include <utility>

#include "../Log.hpp"

namespace Framework::Detail {
	bool CommandCollisionDiagnostics::Collision::operator==(const Collision& other) const {
		return edge == other.edge && virtualKey == other.virtualKey && kind == other.kind &&
			labels == other.labels;
	}

	std::vector<CommandCollisionDiagnostics::Collision>
		CommandCollisionDiagnostics::CollectCollisions(std::vector<ResolvedCommandBinding> bindings) {
		using CollisionKey = std::tuple<KeyEdge, CommandBindingKind, std::uint32_t>;
		std::map<CollisionKey, std::vector<std::string>> byPhysicalKey;

		for (auto& binding : bindings) {
			const char* kind = binding.kind == CommandBindingKind::Setting ? "setting" : "fixed";
			byPhysicalKey[{ binding.edge, binding.kind, binding.virtualKey }]
				.push_back(std::string(kind) + "/" + binding.name);
		}

		std::vector<Collision> collisions;
		for (auto& [key, labels] : byPhysicalKey) {
			if (labels.size() < 2) continue;

			collisions.push_back({ std::get<0>(key), std::get<2>(key), std::get<1>(key),
				std::move(labels) });
		}

		return collisions;
	}

	void CommandCollisionDiagnostics::LogNewCollisions(
		const std::vector<Collision>& collisions,
		const std::vector<Collision>& previousCollisions) {
		for (const Collision& collision : collisions) {
			if (std::find(previousCollisions.begin(), previousCollisions.end(), collision)
				!= previousCollisions.end()) continue;

			LOG_WARNING("[Framework] key collision on VK " << collision.virtualKey << " ("
				<< (collision.edge == KeyEdge::Down ? "down" : "up") << "): '"
				<< collision.labels.front() << "' has first priority when available in the "
				<< (collision.kind == CommandBindingKind::Setting ? "settings" : "fixed-key")
				<< " pass; " << (collision.labels.size() - 1)
				<< " other configured binding(s) share this key in that pass"
				<< std::endl);
		}
	}

	void CommandCollisionDiagnostics::Refresh(std::vector<ResolvedCommandBinding> bindings) {
		auto collisions = CollectCollisions(std::move(bindings));

		std::lock_guard<std::mutex> lock(mutex);
		LogNewCollisions(collisions, previousCollisions);
		previousCollisions = std::move(collisions);
	}
}
