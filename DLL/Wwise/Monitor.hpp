#pragma once

enum AkErrorCode;
enum AkErrorLevel;

namespace Wwise::Monitor {
	AKRESULT PostCode(AkErrorCode in_eError, AkErrorLevel in_eErrorLevel);
}

typedef AKRESULT(__cdecl* tMonitor_PostCode)(AkErrorCode in_eError, AkErrorLevel in_eErrorLevel);