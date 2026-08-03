#pragma once

#include "../Framework/Framework.hpp"

class LoftMod : public Framework::IMod {
public:
	std::string_view Id() const override;
	bool IsEnabled(const Framework::ModContext& c) const override;

	void OnDisabled(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;
	void OnMenuTick(Framework::ModContext& c) override;

private:
	void ApplyAlwaysOn(Framework::ModContext& c);

	bool loftOff = false;
};
