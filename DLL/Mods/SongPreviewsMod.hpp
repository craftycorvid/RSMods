#pragma once

#include "../Framework/Framework.hpp"

class SongPreviewsMod : public Framework::IMod {
public:
	MOD_ID(SongPreviewsMod)

	void OnMenuTick(Framework::ModContext& c) override;

private:
	void SyncState(Framework::ModContext& c);
};
