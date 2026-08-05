#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "Enumeration.hpp"
#include "../Framework/Framework.hpp"

class EnumerationMod : public Framework::IMod {
public:
	std::string_view Id() const override;

	void OnInitialize(Framework::ModContext& c) override;
	void OnSettingsChanged(Framework::ModContext& c) override;
	void OnShutdown(Framework::ModContext& c) override;

private:
	void UpdateSettings(const Framework::ModContext& c);
	void MonitorDlcDirectory();
	bool WaitFor(std::chrono::milliseconds duration);

	std::atomic<bool> automatic_{ false };
	std::atomic<int> intervalMs_{ 0 };
	std::mutex waitMutex_;
	std::condition_variable waitCondition_;
	bool stopping_ = false;
	std::thread monitorThread_;
};
