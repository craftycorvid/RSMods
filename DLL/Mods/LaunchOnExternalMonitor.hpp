#pragma once

#include "../Framework/Framework.hpp"

class LaunchOnExternalMonitorMod : public Framework::IMod {
public:
	std::string_view Id() const override;
	bool IsEnabled(const Framework::ModContext& c) const override;

	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	void MoveOnce(Framework::ModContext& c);
	void SendRocksmithToScreen(int startX, int startY);

	bool moved = false;
};
