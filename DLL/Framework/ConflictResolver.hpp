#pragma once

// Enabled candidates are considered by priority then ID. Winners reserve their resources
// immediately, so conflicts are checked against winners rather than all enabled candidates.

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace Framework::Resolver {

	struct Candidate {
		std::string_view id;
		int priority = 0;
		bool enabled = false;
		const std::vector<std::string>* resources = nullptr; // Exclusive resources needed (may be null/empty).
	};

	inline void Resolve(const std::vector<Candidate>& candidates, std::vector<char>& active) {
		const std::size_t n = candidates.size();
		active.assign(n, 0);

		std::vector<std::size_t> order(n);
		for (std::size_t i = 0; i < n; ++i) order[i] = i;
		std::sort(order.begin(), order.end(), [&](std::size_t x, std::size_t y) {
			if (candidates[x].priority != candidates[y].priority) return candidates[x].priority > candidates[y].priority;
			return candidates[x].id < candidates[y].id; // Stable across TUs, unlike registration order.
		});

		std::unordered_set<std::string> reserved;
		for (std::size_t idx : order) {
			const Candidate& c = candidates[idx];
			if (!c.enabled) continue;

			bool blocked = false;
			if (c.resources) {
				for (const std::string& r : *c.resources) {
					if (reserved.count(r)) { blocked = true; break; }
				}
			}
			if (blocked) continue;

			active[idx] = 1;
			if (c.resources)
				for (const std::string& r : *c.resources) reserved.insert(r);
		}
	}
}
