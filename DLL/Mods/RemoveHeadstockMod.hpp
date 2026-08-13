#pragma once

#include "../Framework/Framework.hpp"

class RemoveHeadstockMod : public Framework::IMod {
public:
	MOD_ID(RemoveHeadstockMod)
	bool IsEnabled(const Framework::ModContext& c) const override;

	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	void ApplyStartup(Framework::ModContext& c);
};
