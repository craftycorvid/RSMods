#pragma once

#include "stdafx.h"
#include "Structs.hpp"

namespace GameState {
	bool IsInSong();
	bool IsMultiplayer();
	std::string CurrentSelectedUser();
	std::string GetSongKey();

	inline std::string lastSongKey = "";

	std::string GetCurrentMenu(bool GameNotLoaded = false);
	void ToggleCB(bool enabled);

	inline static std::string lastMenu = "";
	inline static bool canGetRealMenu = false;
};