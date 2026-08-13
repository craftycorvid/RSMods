#pragma once

#include "../QualityOfLife.hpp"
#include "../Framework/Framework.hpp"

class TwoRTCBypassMod : public Framework::IMod {
public:
	MOD_ID(TwoRTCBypassMod)

	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	void SyncPatch(Framework::ModContext& c);
};
