#pragma once

#include "../Framework/Framework.hpp"

class SongPreviewsMod : public Framework::IMod {
public:
	std::string_view Id() const override;

	void OnMenuTick(Framework::ModContext& c) override;

private:
	void SyncState(Framework::ModContext& c);
};
