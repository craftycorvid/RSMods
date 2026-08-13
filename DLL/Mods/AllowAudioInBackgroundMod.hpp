#pragma once

#include "../Framework/Framework.hpp"

class AllowAudioInBackgroundMod : public Framework::IMod {
public:
	MOD_ID(AllowAudioInBackgroundMod)

	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	void SyncState(Framework::ModContext& c);
};
