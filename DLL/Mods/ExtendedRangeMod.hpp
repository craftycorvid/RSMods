#pragma once

#include "../Framework/Framework.hpp"

// It is always enabled: the string-coloring pass runs every menu and song tick
// regardless of the ExtendedRangeEnabled toggle (that toggle only gates the
// in-song ER detection, which happens inside ERMode).
class ExtendedRangeMod : public Framework::IMod {
public:
	std::string_view Id() const override;

	void OnSongEnter(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;
	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongExit(Framework::ModContext& c) override;

private:
	void ApplyColors();
	static bool SkipERSleep(const Framework::ModContext& c);
};
