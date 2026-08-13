#pragma once

#include "../Framework/Framework.hpp"

class ScreenShotScoresMod : public Framework::IMod {
public:
	MOD_ID(ScreenShotScoresMod)

	void OnMenuTick(Framework::ModContext& c) override;
};
