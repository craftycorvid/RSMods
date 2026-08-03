#pragma once

#include "../Framework/Framework.hpp"

class NonStopPlayTimerMod : public Framework::IMod {
public:
	std::string_view Id() const override;

	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	void EnforceTimer(Framework::ModContext& c);
};
