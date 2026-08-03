#pragma once

#include "../Framework/Framework.hpp"

class RemoveLyricsMod : public Framework::IMod {
public:
	std::string_view Id() const override;
	bool IsEnabled(const Framework::ModContext& c) const override;

	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	void ApplyStartup(Framework::ModContext& c);
};
