#include <iomanip>

#include "../Framework.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
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

	struct RenderBlock {
		bool WaitUntilEntered() {
			std::unique_lock<std::mutex> lock(mutex);
			return changed.wait_for(lock, std::chrono::seconds(2), [this] { return entered; });
		}

		void Release() {
			std::lock_guard<std::mutex> lock(mutex);
			released = true;
			changed.notify_all();
		}

		std::mutex mutex;
		std::condition_variable changed;
		bool entered = false;
		bool released = false;
	};
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
		bool subscribeRender = false;
		bool throwInRender = false;
		bool recordDestruction = false;
		std::shared_ptr<RenderBlock> renderBlock;
		std::string commandSetting;
		Framework::Availability availability = Framework::Availability::Active;
		int commandCalls = 0;
		bool lastCommandControl = false;

		explicit TestMod(std::string i) : id(std::move(i)), tag(id) {}
		~TestMod() override {
			if (recordDestruction)
				RecordEvent(tag + ":destroy");
		}

		std::string_view Id() const override { return id; }
		bool IsEnabled(const Framework::ModContext&) const override { return enabled; }
		int Priority() const override { return prio; }

		std::vector<std::string_view> ClaimsExclusive() const override {
			std::vector<std::string_view> v; for (auto& s : claimStrings) v.push_back(s); return v;
		}

		void OnInitialize(Framework::ModContext& c) override {
			Rec("OnInitialize");
			if (!commandSetting.empty()) {
				c.Commands().BindSetting(commandSetting, Framework::KeyEdge::Up, availability,
					[this](Framework::ModContext&, const Framework::KeyEvent& event) {
						++commandCalls;
						lastCommandControl = event.control;
					});
			}
			if (subscribeRender)
				c.Render().OnEndScene([this](IDirect3DDevice9*) {
					RecordEvent(tag + ":render");
					if (renderBlock) {
						std::unique_lock<std::mutex> lock(renderBlock->mutex);
						renderBlock->entered = true;
						renderBlock->changed.notify_all();
						renderBlock->changed.wait(lock, [this] { return renderBlock->released; });
						lock.unlock();
						RecordEvent(tag + ":render-complete");
					}
					if (throwInRender) throw std::runtime_error("render boom");
				});
		}
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
	reg.Tick(GamePhase::Menu);
	Expect(!Has("A:OnEnabled") && Has("B:OnEnabled"), "initialization fault prevents only A from activating");
	reg.Shutdown();
}

static void Test_ActivateInMenu() {
	ClearEvents();
	ModRegistry reg;
	Add(reg, "M");
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Menu);
	ExpectSeq({ "M:OnEnabled", "M:OnTick", "M:OnMenuTick" }, "activate in menu: no song edges");
	reg.Shutdown();
}

static void Test_EnableMidSong() {
	ClearEvents();
	ModRegistry reg;
	Add(reg, "S");
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Song);
	ExpectSeq({ "S:OnEnabled", "S:OnSongEnter", "S:OnTick", "S:OnSongTick" }, "enable mid-song fires OnSongEnter");
	reg.Shutdown();
}

static void Test_DisableMidSong() {
	ClearEvents();
	ModRegistry reg;
	TestMod* s = Add(reg, "S");
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Song);
	ClearEvents();
	s->enabled = false;
	reg.Tick(GamePhase::Song);
	ExpectSeq({ "S:OnSongExit", "S:OnDisabled" }, "disable mid-song: OnSongExit before OnDisabled");
	reg.Shutdown();
}

static void Test_ReenableAfterDisable() {
	ClearEvents();
	ModRegistry reg;
	TestMod* s = Add(reg, "S");
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu);
	s->enabled = false;
	reg.Tick(GamePhase::Menu);
	ClearEvents();
	s->enabled = true;
	reg.Tick(GamePhase::Song);
	ExpectSeq({ "S:OnEnabled", "S:OnSongEnter", "S:OnTick", "S:OnSongTick" },
		"disabled mod reactivates through the shared inactive state");
	reg.Shutdown();
}

static void Test_DisabledSameTickAsSongExit() {
	ClearEvents();
	ModRegistry reg;
	TestMod* s = Add(reg, "S");
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Song);
	ClearEvents();
	s->enabled = false;
	reg.Tick(GamePhase::Menu);
	Expect(Has("S:OnSongExit") && Has("S:OnDisabled"), "OnSongExit not missed when disabled on song-exit tick");
	Expect(IndexOf("S:OnSongExit") < IndexOf("S:OnDisabled"), "OnSongExit ordered before OnDisabled");
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
	reg.Tick(GamePhase::Menu);
	Expect(Has("A:OnEnabled") && !Has("B:OnEnabled"), "A active alone");
	ClearEvents();
	b->enabled = true;
	reg.Tick(GamePhase::Menu);
	Expect(Has("A:OnDisabled"), "loser A gets OnDisabled on suppression");
	Expect(Has("B:OnEnabled"), "winner B activates");
	Expect(IndexOf("A:OnDisabled") < IndexOf("B:OnEnabled"), "deactivation precedes activation (resource handoff)");
	reg.Shutdown();
}

static void Test_OnEnabledThrowsFaults() {
	ClearEvents();
	ModRegistry reg;
	TestMod* f = Add(reg, "F"); f->throwOn = "OnEnabled";
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Menu);
	ExpectSeq({ "F:OnEnabled" }, "OnEnabled throw: no tick hooks run");
	ClearEvents();
	reg.Tick(GamePhase::Menu);
	Expect(EventsEmpty(), "faulted mod produces no further events");
	reg.Shutdown();
}

static void Test_OnSongEnterThrowsShortCircuits() {
	ClearEvents();
	ModRegistry reg;
	TestMod* s = Add(reg, "S"); s->throwOn = "OnSongEnter";
	reg.DispatchInitialize();
	ClearEvents();
	reg.Tick(GamePhase::Song);
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
	reg.Tick(GamePhase::Menu);
	ExpectSeq({ "T:OnEnabled", "T:OnTick", "T:OnDisabled" }, "tick exception faults and tears down immediately");
	ClearEvents();
	reg.Tick(GamePhase::Menu);
	Expect(EventsEmpty(), "tick-faulted mod produces no further events");
	reg.Shutdown();
}

static void Test_SettingsAppliedThenNotifiedOnTick() {
	ClearEvents();
	ModRegistry reg;
	Add(reg, "M");
	reg.DispatchInitialize();
	ClearEvents();
	reg.EnqueueSettingsUpdate([] { RecordEvent("SETTINGS:apply-1"); });
	reg.EnqueueSettingsUpdate([] { RecordEvent("SETTINGS:apply-2"); });
	Expect(!Has("SETTINGS:apply-1") && !Has("SETTINGS:apply-2"),
		"settings not applied on the message thread (before tick)");
	reg.Tick(GamePhase::Menu);
	Expect(Has("SETTINGS:apply-1") && Has("SETTINGS:apply-2") && Has("M:OnSettingsChanged"),
		"settings batch applied and mods notified on tick");
	Expect(IndexOf("SETTINGS:apply-1") < IndexOf("SETTINGS:apply-2"),
		"settings closures preserve FIFO order within a batch");
	Expect(IndexOf("SETTINGS:apply-2") < IndexOf("M:OnSettingsChanged"),
		"complete settings batch precedes notification");
	Expect(IndexOf("M:OnSettingsChanged") < IndexOf("M:OnEnabled"), "notify precedes activation resolve");
	reg.Shutdown();
}

static Framework::KeyEvent TestKeyEvent(unsigned int key, bool control = false) {
	Framework::KeyEvent event;
	event.virtualKey = key;
	event.edge = Framework::KeyEdge::Up;
	event.control = control;
	return event;
}

static void Test_KeyAvailabilityTracksRegistryLifecycle() {
	Framework::Commands().SetKeyResolver([](std::string_view setting) {
		return setting == "TestEffectiveCommand" ? 80u : setting == "TestInitializedCommand" ? 81u : 0u;
	});

	ModRegistry reg;
	TestMod* effective = Add(reg, "CommandEffective", false);
	effective->commandSetting = "TestEffectiveCommand";
	TestMod* initialized = Add(reg, "CommandInitialized", false);
	initialized->commandSetting = "TestInitializedCommand";
	initialized->availability = Framework::Availability::Initialized;
	reg.DispatchInitialize();

	Framework::Commands().Enqueue(TestKeyEvent(80, true));
	Framework::Commands().Enqueue(TestKeyEvent(81, true));
	reg.DispatchCommands(GamePhase::Menu, true);
	Expect(effective->commandCalls == 0 && initialized->commandCalls == 1 && initialized->lastCommandControl,
		"registry exposes initialized binding but gates inactive active-only binding");

	effective->enabled = true;
	reg.Tick(GamePhase::Menu);
	Framework::Commands().Enqueue(TestKeyEvent(80, true));
	reg.DispatchCommands(GamePhase::Menu, true);
	Expect(effective->commandCalls == 1 && effective->lastCommandControl,
		"registry activation enables command with captured modifier state");

	effective->enabled = false;
	reg.Tick(GamePhase::Menu);
	Framework::Commands().Enqueue(TestKeyEvent(80));
	reg.DispatchCommands(GamePhase::Menu, true);
	Expect(effective->commandCalls == 1, "registry deactivation disables effective command before delivery");
	reg.Shutdown();
}

static void Test_ConflictSuppressionGatesEffectiveCommand() {
	Framework::Commands().SetKeyResolver([](std::string_view setting) {
		return setting == "SuppressedCommand" ? 82u : 0u;
	});

	ModRegistry reg;
	TestMod* suppressed = Add(reg, "Suppressed", true, 0);
	suppressed->claimStrings = { "R" };
	suppressed->commandSetting = "SuppressedCommand";
	TestMod* winner = Add(reg, "Winner", true, 10);
	winner->claimStrings = { "R" };
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu);

	Framework::Commands().Enqueue(TestKeyEvent(82));
	reg.DispatchCommands(GamePhase::Menu, true);
	Expect(suppressed->commandCalls == 0,
		"conflict-suppressed mod cannot bypass exclusive resource through effective command");
	reg.Shutdown();
}

static void Test_DuplicateIdRejected() {
	ClearEvents();
	ModRegistry reg;
	TestMod* a = Add(reg, "Dup"); a->tag = "d1";
	auto dup = std::make_unique<TestMod>("Dup"); dup->tag = "d2";
	reg.Register(std::move(dup));
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu);
	Expect(Has("d1:OnEnabled"), "first Dup registration active");
	Expect(!Has("d2:OnInitialize") && !Has("d2:OnEnabled"), "duplicate Id rejected (never runs)");
	reg.Shutdown();
}

static void Test_RenderModActivationGating() {
	ClearEvents();
	ModRegistry reg;
	TestMod* v = Add(reg, "V");
	v->subscribeRender = true;
	reg.DispatchInitialize();
	IDirect3DDevice9* dev = nullptr;

	Framework::Hooks::Render().DispatchEndScene(dev);
	Expect(!Has("V:render"), "callback not invoked while mod inactive");

	reg.Tick(GamePhase::Menu);
	ClearEvents();
	Framework::Hooks::Render().DispatchEndScene(dev);
	Expect(Has("V:render"), "callback invoked once mod active");

	v->enabled = false;
	reg.Tick(GamePhase::Menu);
	ClearEvents();
	Framework::Hooks::Render().DispatchEndScene(dev);
	Expect(!Has("V:render"), "callback not invoked after deactivation");

	reg.Shutdown();
	ClearEvents();
	Framework::Hooks::Render().DispatchEndScene(dev);
	Expect(!Has("V:render"), "callback removed after shutdown");
}

static void Test_RemoveModDropsPublishedCallbacks() {
	TestMod mod("render-test");
	int calls = 0;
	auto& hooks = Framework::Hooks::Render();

	hooks.Subscribe(&mod, [&](IDirect3DDevice9*) { ++calls; });
	hooks.SetModActive(&mod, true);
	hooks.DispatchEndScene(nullptr);
	Expect(calls == 1, "direct render subscription runs while mod is active");

	hooks.RemoveMod(&mod);
	hooks.SetModActive(&mod, true); // Simulate a later registration reusing the same mod address.
	hooks.DispatchEndScene(nullptr);
	Expect(calls == 1, "removed callback is absent from the published snapshot");

	hooks.RemoveMod(&mod);
}

static void Test_RenderCallbackThrowDisablesMod() {
	ClearEvents();
	ModRegistry reg;
	TestMod* v = Add(reg, "V");
	v->subscribeRender = true;
	v->throwInRender = true;
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu);

	IDirect3DDevice9* dev = nullptr;
	Framework::Hooks::Render().DispatchEndScene(dev);
	Expect(Has("V:render"), "throwing render callback still ran once");

	ClearEvents();
	reg.Tick(GamePhase::Menu);
	Expect(Has("V:OnDisabled"), "render-faulted mod is torn down (OnDisabled)");

	ClearEvents();
	Framework::Hooks::Render().DispatchEndScene(dev);
	Expect(!Has("V:render"), "render-faulted mod no longer receives callbacks");
	reg.Shutdown();
}

static void Test_ResourceHandoffWaitsForRenderQuiescence() {
	ClearEvents();
	ModRegistry reg;
	TestMod* oldMod = Add(reg, "Old", true, 1);
	TestMod* newMod = Add(reg, "New", false, 2);
	oldMod->claimStrings = { "R" };
	newMod->claimStrings = { "R" };
	oldMod->subscribeRender = true;
	oldMod->renderBlock = std::make_shared<RenderBlock>();
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu);
	ClearEvents();

	std::thread renderThread([] {
		Framework::Hooks::Render().DispatchEndScene(nullptr);
	});
	Expect(oldMod->renderBlock->WaitUntilEntered(), "old mod's render callback entered");

	newMod->enabled = true;
	reg.Tick(GamePhase::Menu);
	Expect(!Has("Old:OnDisabled"), "old mod teardown waits for its render callback");
	Expect(!Has("New:OnEnabled"), "replacement waits while the old mod holds the resource");
	Expect(!Has("Old:OnTick"), "deactivating mod no longer receives tick hooks");

	oldMod->renderBlock->Release();
	renderThread.join();
	reg.Tick(GamePhase::Menu);
	Expect(Has("Old:OnDisabled") && Has("New:OnEnabled"), "handoff completes after render quiescence");
	Expect(IndexOf("Old:OnDisabled") < IndexOf("New:OnEnabled"), "old mod releases state before replacement activation");
	reg.Shutdown();
}

static void Test_RenderFaultUpgradesDeferredTeardown() {
	ClearEvents();
	ModRegistry reg;
	TestMod* mod = Add(reg, "V");
	mod->subscribeRender = true;
	mod->throwInRender = true;
	mod->renderBlock = std::make_shared<RenderBlock>();
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu);
	ClearEvents();

	std::thread renderThread([] {
		Framework::Hooks::Render().DispatchEndScene(nullptr);
	});
	Expect(mod->renderBlock->WaitUntilEntered(), "faulting render callback entered");
	mod->enabled = false;
	reg.Tick(GamePhase::Menu);
	Expect(!Has("V:OnDisabled"), "disabled mod waits for its render callback");

	mod->renderBlock->Release();
	renderThread.join();
	reg.Tick(GamePhase::Menu);
	Expect(Has("V:OnDisabled"), "deferred render fault completes teardown");

	ClearEvents();
	mod->enabled = true;
	reg.Tick(GamePhase::Menu);
	Expect(!Has("V:OnEnabled"), "render fault upgrades deferred teardown to terminal Faulted");
	reg.Shutdown();
}

static void Test_ShutdownWaitsForRenderQuiescence() {
	ClearEvents();
	ModRegistry reg;
	TestMod* mod = Add(reg, "V");
	mod->subscribeRender = true;
	mod->recordDestruction = true;
	mod->renderBlock = std::make_shared<RenderBlock>();
	reg.DispatchInitialize();
	reg.Tick(GamePhase::Menu);
	ClearEvents();

	std::thread renderThread([] {
		Framework::Hooks::Render().DispatchEndScene(nullptr);
	});
	Expect(mod->renderBlock->WaitUntilEntered(), "shutdown test render callback entered");

	std::atomic<bool> shutdownStarted{ false };
	std::atomic<bool> shutdownFinished{ false };
	std::thread shutdownThread([&] {
		shutdownStarted.store(true, std::memory_order_release);
		reg.Shutdown();
		shutdownFinished.store(true, std::memory_order_release);
	});
	while (!shutdownStarted.load(std::memory_order_acquire))
		std::this_thread::yield();
	std::this_thread::sleep_for(std::chrono::milliseconds(30));
	Expect(!shutdownFinished.load(std::memory_order_acquire), "shutdown remains blocked while callback is in flight");
	Expect(!Has("V:OnDisabled") && !Has("V:destroy"), "shutdown preserves mod state while callback is in flight");

	mod->renderBlock->Release();
	renderThread.join();
	shutdownThread.join();
	Expect(Has("V:render-complete") && Has("V:OnDisabled") && Has("V:OnShutdown") && Has("V:destroy"),
		"shutdown completes lifecycle and destruction after quiescence");
	Expect(IndexOf("V:render-complete") < IndexOf("V:OnDisabled") &&
		IndexOf("V:OnDisabled") < IndexOf("V:OnShutdown") &&
		IndexOf("V:OnShutdown") < IndexOf("V:destroy"),
		"shutdown ordering keeps callback completion before teardown and destruction");
}

int main() {
	std::cout << "ModRegistry state-machine tests\n";

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
	Test_KeyAvailabilityTracksRegistryLifecycle();
	Test_ConflictSuppressionGatesEffectiveCommand();
	Test_DuplicateIdRejected();
	Test_RenderModActivationGating();
	Test_RemoveModDropsPublishedCallbacks();
	Test_RenderCallbackThrowDisablesMod();
	Test_ResourceHandoffWaitsForRenderQuiescence();
	Test_RenderFaultUpgradesDeferredTeardown();
	Test_ShutdownWaitsForRenderQuiescence();

	std::cout << (g_failures == 0 ? "\nALL PASS\n" : "\nFAILURES: " + std::to_string(g_failures) + "\n");
	return g_failures == 0 ? 0 : 1;
}
