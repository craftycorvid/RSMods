#include "../ConflictResolver.hpp"

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using Framework::Resolver::Candidate;

namespace {
	int g_failures = 0;

	struct Mod {
		std::string id;
		int priority = 0;
		bool enabled = true;
		std::vector<std::string> resources;
	};

	std::set<std::string> ActiveIds(std::vector<Mod>& mods) {
		std::vector<Candidate> candidates(mods.size());
		for (size_t i = 0; i < mods.size(); ++i) {
			candidates[i].id = mods[i].id;
			candidates[i].priority = mods[i].priority;
			candidates[i].enabled = mods[i].enabled;
			candidates[i].resources = &mods[i].resources;
		}
		std::vector<char> active;
		Framework::Resolver::Resolve(candidates, active);

		std::set<std::string> result;
		for (size_t i = 0; i < mods.size(); ++i)
			if (active[i]) result.insert(mods[i].id);
		return result;
	}

	void Check(const std::string& name, const std::set<std::string>& got, const std::set<std::string>& want) {
		if (got == want) {
			std::cout << "  PASS  " << name << "\n";
			return;
		}
		++g_failures;
		auto join = [](const std::set<std::string>& s) {
			std::string out;
			for (const auto& x : s) { if (!out.empty()) out += ","; out += x; }
			return out.empty() ? "{}" : out;
		};
		std::cout << "  FAIL  " << name << "  got={" << join(got) << "} want={" << join(want) << "}\n";
	}

	std::string ConflictResource(const std::string& a, const std::string& b) {
		return "conflict:" + (a < b ? a + "|" + b : b + "|" + a);
	}
}

int main() {
	std::cout << "ConflictResolver tests\n";

	{
		std::vector<Mod> mods = { {"A"}, {"B"} };
		Check("compatible mods both active", ActiveIds(mods), { "A", "B" });
	}

	{
		std::vector<Mod> mods = {
			{"A", 10, true, {"R"}},
			{"B", 0,  true, {"R"}},
		};
		Check("resource: higher priority wins", ActiveIds(mods), { "A" });
	}

	{
		std::vector<Mod> mods = {
			{"A", 0, true, { ConflictResource("A","B") }},
			{"B", 2, true, { ConflictResource("A","B"), ConflictResource("B","C") }},
			{"C", 3, true, { ConflictResource("B","C") }},
		};
		Check("conflict chain resolves to C and A", ActiveIds(mods), { "A", "C" });
	}

	{
		std::vector<Mod> mods = {
			{"B", 5, true, {"R"}},
			{"A", 5, true, {"R"}},
		};
		Check("equal priority tie: smaller id wins", ActiveIds(mods), { "A" });
	}

	{
		std::vector<Mod> mods = {
			{"A", 10, false, {"R"}},
			{"B", 0,  true,  {"R"}},
		};
		Check("disabled claimant frees resource", ActiveIds(mods), { "B" });
	}

	{
		std::vector<Mod> mods = {
			{"A", 5,  true, {"R1", "R2"}},
			{"B", 10, true, {"R2"}},
			{"C", 1,  true, {"R1"}},
		};
		Check("multi-resource greedy blocks A", ActiveIds(mods), { "B", "C" });
	}

	{
		std::vector<Mod> mods = {
			{"DropPedal",   10, true, {"tuning-controller"}},
			{"MidiAutoTune", 0, true, {"tuning-controller"}},
		};
		Check("DropPedal wins tuning-controller", ActiveIds(mods), { "DropPedal" });
	}

	std::cout << (g_failures == 0 ? "\nALL PASS\n" : "\nFAILURES: " + std::to_string(g_failures) + "\n");
	return g_failures == 0 ? 0 : 1;
}
