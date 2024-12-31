#pragma once

namespace Wwise {
	bool IsRestoreSinkRequested(void);
	bool IsUsingDummySink(void);
}

typedef bool(__cdecl* tIsRestoreSinkRequested)(void);
typedef bool(__cdecl* tIsUsingDummySink)(void);