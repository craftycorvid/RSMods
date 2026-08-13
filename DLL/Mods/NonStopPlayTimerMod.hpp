#pragma once

#include "../Framework/Framework.hpp"

class NonStopPlayTimerMod : public Framework::IMod {
public:
	MOD_ID(NonStopPlayTimerMod)

	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	void EnforceTimer(Framework::ModContext& c);
};
