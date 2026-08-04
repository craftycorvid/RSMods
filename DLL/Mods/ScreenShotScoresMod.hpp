#pragma once

#include "../Framework/Framework.hpp"

class ScreenShotScoresMod : public Framework::IMod {
public:
	std::string_view Id() const override;

	void OnMenuTick(Framework::ModContext& c) override;
};
