#include <iomanip>
#include "../../RSColor.h"
struct GameLoopState {};

#include "../Framework.hpp"

#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

using Framework::ModRegistry;
using Framework::GamePhase;

namespace {
	std::vector<std::string> g_events;
	std::mutex g_eventsMutex;

	void RecordEvent(std::string event) {
		std::lock_guard<std::mutex> lock(g_eventsMutex);
		g_events.push_back(std::move(event));
	}

	void ClearEvents() {
		std::lock_guard<std::mutex> lock(g_eventsMutex);
		g_events.clear();
	}

	int IndexOf(const std::string& e) {
		std::lock_guard<std::mutex> lock(g_eventsMutex);
		for (int i = 0; i < (int)g_events.size(); ++i) if (g_events[i] == e) return i;
		return -1;
	}
	bool Has(const std::string& e) { return IndexOf(e) >= 0; }
	bool EventsEmpty() {
		std::lock_guard<std::mutex> lock(g_eventsMutex);
		return g_events.empty();
	}

	int g_failures = 0;

	void Expect(bool cond, const std::string& name) {
		std::cout << (cond ? "  PASS  " : "  FAIL  ") << name << "\n";
		if (!cond) ++g_failures;
	}

	void ExpectSeq(const std::vector<std::string>& want, const std::string& name) {
		std::vector<std::string> got;
		{
			std::lock_guard<std::mutex> lock(g_eventsMutex);
			got = g_events;
		}
		const bool ok = got == want;
		Expect(ok, name);
		if (!ok) {
			auto join = [](const std::vector<std::string>& v) {
				std::string s; for (auto& x : v) { if (!s.empty()) s += ", "; s += x; } return s;
			};
			std::cout << "        got:  [" << join(got) << "]\n";
			std::cout << "        want: [" << join(want) << "]\n";
		}
	}
}

namespace {
	class TestMod : public Framework::IMod {
	public:
		std::string id;
		std::string tag;
		bool enabled = true;
		int prio = 0;
		std::vector<std::string> claimStrings;
		std::string throwOn;

		explicit TestMod(std::string i) : id(std::move(i)), tag(id) {}

		std::string_view Id() const override { return id; }
		bool IsEnabled(const Framework::ModContext&) const override { return enabled; }
		int Priority() const override { return prio; }

		std::vector<std::string_view> ClaimsExclusive() const override {
			std::vector<std::string_view> v; for (auto& s : claimStrings) v.push_back(s); return v;
		}

		void OnInitialize(Framework::ModContext&) override { Rec("OnInitialize"); }
		void OnShutdown(Framework::ModContext&) override { Rec("OnShutdown"); }
		void OnSettingsChanged(Framework::ModContext&) override { Rec("OnSettingsChanged"); }
		void OnEnabled(Framework::ModContext&) override { Rec("OnEnabled"); }
		void OnDisabled(Framework::ModContext&) override { Rec("OnDisabled"); }
		void OnTick(Framework::ModContext&) override { Rec("OnTick"); }
		void OnMenuTick(Framework::ModContext&) override { Rec("OnMenuTick"); }
		void OnSongEnter(Framework::ModContext&) override { Rec("OnSongEnter"); }
		void OnSongTick(Framework::ModContext&) override { Rec("OnSongTick"); }
		void OnSongExit(Framework::ModContext&) override { Rec("OnSongExit"); }

	private:
		void Rec(const char* hook) {
			RecordEvent(tag + ":" + hook);
			if (throwOn == hook) throw std::runtime_error("injected failure in " + std::string(hook));
		}
	};

	TestMod* Add(ModRegistry& reg, const std::string& id, bool enabled = true, int prio = 0) {
		auto m = std::make_unique<TestMod>(id);
		m->enabled = enabled;
		m->prio = prio;
		TestMod* raw = m.get();
		reg.Register(std::move(m));
		return raw;
	}

	GameLoopState g_gls;
}

static void Test_InitializeFaultIsIsolated() {
	ClearEvents();
	ModRegistry reg;
	TestMod* a = Add(reg, "A"); a->throwOn = "OnInitialize";
	Add(reg, "B");
	reg.DispatchInitialize();
	Expect(Has("A:OnInitialize"), "A initialization attempted");
	Expect(Has("B:OnInitialize"), "B unaffected by A's initialization fault");
	ClearEvents();
	reg.Tick(GamePhase::Menu, g_gls);
	Expect(!Has("A:OnEnabled") && Has("B:OnEnabled"), "initialization fault prevents only A from activating");
	reg.Shutdown();
}

static void Test_ActivateInMenu() {
	ClearEvents();
	ModRegistry reg;
	Add(reg, "M");
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Menu, g_gls);
	ExpectSeq({ "M:OnEnabled", "M:OnTick", "M:OnMenuTick" }, "activate in menu: no song edges");
	reg.Shutdown();
}

static void Test_EnableMidSong() {
	ClearEvents();
	ModRegistry reg;
	Add(reg, "S");
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Song, g_gls);
	ExpectSeq({ "S:OnEnabled", "S:OnSongEnter", "S:OnTick", "S:OnSongTick" }, "enable mid-song fires OnSongEnter");
	reg.Shutdown();
}

static void Test_DisableMidSong() {
	ClearEvents();
	ModRegistry reg;
	TestMod* s = Add(reg, "S");
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Song, g_gls);
	ClearEvents();
	s->enabled = false;
	reg.Tick(GamePhase::Song, g_gls);
	ExpectSeq({ "S:OnSongExit", "S:OnDisabled" }, "disable mid-song: OnSongExit before OnDisabled");
	reg.Shutdown();
}

static void Test_ReenableAfterDisable() {
	ClearEvents();
	ModRegistry reg;
	TestMod* s = Add(reg, "S");
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu, g_gls);
	s->enabled = false;
	reg.Tick(GamePhase::Menu, g_gls);
	ClearEvents();
	s->enabled = true;
	reg.Tick(GamePhase::Song, g_gls);
	ExpectSeq({ "S:OnEnabled", "S:OnSongEnter", "S:OnTick", "S:OnSongTick" },
		"disabled mod reactivates through the shared inactive state");
	reg.Shutdown();
}

static void Test_DisabledSameTickAsSongExit() {
	ClearEvents();
	ModRegistry reg;
	TestMod* s = Add(reg, "S");
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Song, g_gls);
	ClearEvents();
	s->enabled = false;
	reg.Tick(GamePhase::Menu, g_gls);
	Expect(Has("S:OnSongExit") && Has("S:OnDisabled"), "OnSongExit not missed when disabled on song-exit tick");
	Expect(IndexOf("S:OnSongExit") < IndexOf("S:OnDisabled"), "OnSongExit ordered before OnDisabled");
	reg.Shutdown();
}

static void Test_OnEnabledThrowsFaults() {
	ClearEvents();
	ModRegistry reg;
	TestMod* f = Add(reg, "F"); f->throwOn = "OnEnabled";
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Menu, g_gls);
	ExpectSeq({ "F:OnEnabled" }, "OnEnabled throw: no tick hooks run, no OnDisabled");
	ClearEvents();
	reg.Tick(GamePhase::Menu, g_gls);
	Expect(EventsEmpty(), "faulted mod produces no further events");
	reg.Shutdown();
}

static void Test_OnSongEnterThrowsShortCircuits() {
	ClearEvents();
	ModRegistry reg;
	TestMod* s = Add(reg, "S"); s->throwOn = "OnSongEnter";
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Song, g_gls);
	ExpectSeq({ "S:OnEnabled", "S:OnSongEnter", "S:OnDisabled" },
		"OnSongEnter throw faults immediately and skips tick hooks");
	reg.Shutdown();
}

static void Test_TickFailureFaultsImmediately() {
	ClearEvents();
	ModRegistry reg;
	TestMod* t = Add(reg, "T"); t->throwOn = "OnTick";
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Menu, g_gls);
	ExpectSeq({ "T:OnEnabled", "T:OnTick", "T:OnDisabled" }, "tick exception faults and tears down immediately");
	ClearEvents();
	reg.Tick(GamePhase::Menu, g_gls);
	Expect(EventsEmpty(), "tick-faulted mod produces no further events");
	reg.Shutdown();
}

static void Test_SettingsAppliedThenNotifiedOnTick() {
	ClearEvents();
	ModRegistry reg;
	Add(reg, "M");
	reg.DispatchInitialize();
	ClearEvents();
	reg.EnqueueSettingsUpdate([] { RecordEvent("SETTINGS:apply"); });
	Expect(!Has("SETTINGS:apply"), "settings not applied on the message thread (before tick)");
	reg.Tick(GamePhase::Menu, g_gls);
	Expect(Has("SETTINGS:apply") && Has("M:OnSettingsChanged"), "settings applied and mods notified on tick");
	Expect(IndexOf("SETTINGS:apply") < IndexOf("M:OnSettingsChanged"), "apply precedes notify");
	Expect(IndexOf("M:OnSettingsChanged") < IndexOf("M:OnEnabled"), "notify precedes activation resolve");
	reg.Shutdown();
}

static void Test_DuplicateIdRejected() {
	ClearEvents();
	ModRegistry reg;
	TestMod* a = Add(reg, "Dup"); a->tag = "d1";
	auto dup = std::make_unique<TestMod>("Dup"); dup->tag = "d2";
	reg.Register(std::move(dup));
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu, g_gls);
	Expect(Has("d1:OnEnabled"), "first Dup registration active");
	Expect(!Has("d2:OnInitialize") && !Has("d2:OnEnabled"), "duplicate Id rejected (never runs)");
	reg.Shutdown();
}

static void Test_ConflictSuppressionOrder() {
	ClearEvents();
	ModRegistry reg;
	TestMod* a = Add(reg, "A", true, 1);
	TestMod* b = Add(reg, "B", false, 2);
	a->claimStrings = { "R" };
	b->claimStrings = { "R" };
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu, g_gls);
	Expect(Has("A:OnEnabled") && !Has("B:OnEnabled"), "A active alone");
	ClearEvents();
	b->enabled = true;
	reg.Tick(GamePhase::Menu, g_gls);
	Expect(Has("A:OnDisabled"), "loser A gets OnDisabled on suppression");
	Expect(Has("B:OnEnabled"), "winner B activates");
	Expect(IndexOf("A:OnDisabled") < IndexOf("B:OnEnabled"), "deactivation precedes activation (resource handoff)");
	reg.Shutdown();
}

int main() {
	std::cout << "ModRegistry state-machine tests (+ conflicts)\n";

	Test_InitializeFaultIsIsolated();
	Test_ActivateInMenu();
	Test_EnableMidSong();
	Test_DisableMidSong();
	Test_ReenableAfterDisable();
	Test_DisabledSameTickAsSongExit();
	Test_ConflictSuppressionOrder();
	Test_OnEnabledThrowsFaults();
	Test_OnSongEnterThrowsShortCircuits();
	Test_TickFailureFaultsImmediately();
	Test_SettingsAppliedThenNotifiedOnTick();
	Test_DuplicateIdRejected();

	std::cout << (g_failures == 0 ? "\nALL PASS\n" : "\nFAILURES: " + std::to_string(g_failures) + "\n");
	return g_failures == 0 ? 0 : 1;
}
