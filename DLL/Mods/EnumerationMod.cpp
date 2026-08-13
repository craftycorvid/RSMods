#include "../stdafx.h"
#include "EnumerationMod.hpp"

using Framework::ModContext;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Settings::When;
namespace Setting = Settings::Setting;

std::string_view EnumerationMod::Id() const {
	return "ForceReEnumeration";
}

void EnumerationMod::OnInitialize(ModContext& c) {
	c.Commands().BindSetting(
		"ForceReEnumerationKey",
		KeyEdge::Up,
		Availability::Active,
		[](ModContext&, const KeyEvent&) {
			Enumeration::ForceEnumeration();
		},
		[](const ModContext& context, const KeyEvent&) {
			return context.When(Setting::ForceReEnumerationEnabled) == When::Manual;
		},
		"Force Enumeration");

	UpdateSettings(c);
	monitorThread_ = std::thread(&EnumerationMod::MonitorDlcDirectory, this);
}

void EnumerationMod::OnSettingsChanged(ModContext& c) {
	UpdateSettings(c);
}

// Runs on the main thread. ForceEnumeration() writes game memory, so it must never be
// called from the monitor thread; the monitor only flags that the DLC count changed.
void EnumerationMod::OnTick(ModContext&) {
	if (enumerationRequested_.exchange(false))
		Enumeration::ForceEnumeration();
}

void EnumerationMod::OnShutdown(ModContext&) {
	{
		std::lock_guard lock(waitMutex_);
		stopping_ = true;
	}

	waitCondition_.notify_one();
	if (monitorThread_.joinable()) monitorThread_.join();
}

void EnumerationMod::UpdateSettings(const ModContext& c) {
	automatic_.store(c.When(Setting::ForceReEnumerationEnabled) == When::Automatic);

	intervalMs_.store(c.Int(Setting::CheckForNewSongsInterval));
}

void EnumerationMod::MonitorDlcDirectory() {
	while (!GameState::GameLoaded) {
		if (WaitFor(std::chrono::seconds(5))) return;
	}

	int previousDlcCount = Enumeration::GetCurrentDLCCount();

	while (true) {
		const auto interval = std::chrono::milliseconds(intervalMs_.load());
		if (WaitFor(interval)) return;
		if (!automatic_.load()) continue;

		const int currentDlcCount = Enumeration::GetCurrentDLCCount();
		if (previousDlcCount != currentDlcCount) enumerationRequested_.store(true);
		previousDlcCount = currentDlcCount;
	}
}

bool EnumerationMod::WaitFor(std::chrono::milliseconds duration) {
	std::unique_lock lock(waitMutex_);

	return waitCondition_.wait_for(lock, duration, [this] { return stopping_; });
}

static Framework::ModRegistrar<EnumerationMod> _enumerationReg;
