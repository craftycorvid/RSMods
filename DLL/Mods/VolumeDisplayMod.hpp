#pragma once

#include "../Framework/Framework.hpp"

class VolumeDisplayMod : public Framework::IMod {
public:
	std::string_view Id() const override;

	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	void SyncDisplay(Framework::ModContext& c);
	bool MoreThanThreeSecondsPassed() const;
};
