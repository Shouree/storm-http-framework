#include "stdafx.h"
#include "Debug.h"

namespace util {
	std::wostream &debugStream() {
		return std::wcout;
	}
}

#ifdef WINDOWS

#ifdef DEBUG
class CrtInit {
public:
	CrtInit() {
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

		// Check heap consistency at each allocation.
		// int f = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		// _CrtSetDbgFlag((f & 0x0000FFFF) | _CRTDBG_CHECK_ALWAYS_DF);
	}

	~CrtInit() {
		int f = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		_CrtSetDbgFlag(f | _CRTDBG_LEAK_CHECK_DF);
	}
};
#endif

void initDebug() {
#ifdef DEBUG
	static CrtInit initer;
#endif
}

#endif

#ifdef POSIX

void initDebug() {}

#endif
