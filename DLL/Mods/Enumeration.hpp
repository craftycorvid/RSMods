#pragma once

namespace Enumeration {
	void ForceEnumeration();
	int GetCurrentDLCCount();
	int GetFileCount(std::filesystem::path path);
	void HookEnumerationService();
	inline uint32_t* rsSteamServiceFlagsPtr = nullptr;
};
