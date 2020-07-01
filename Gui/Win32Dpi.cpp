#include "stdafx.h"
#include "Win32Dpi.h"

namespace gui {
	// Default DPI (i.e. 100%, no scaling).
	const Nat defaultDpi = 96;
}

#ifdef GUI_WIN32

namespace gui {

	// From the MSDN documentation.
#define DPI_AWARENESS_CONTEXT_UNAWARE              ((DWORD)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE         ((DWORD)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((DWORD)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DWORD)-4)
#define DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED    ((DWORD)-5)

	enum PROCESS_DPI_AWARENESS {
		PROCESS_DPI_UNAWARE,
		PROCESS_SYSTEM_DPI_AWARE,
		PROCESS_PER_MONITOR_DPI_AWARE
	};

	// DPI awareness mode.
	enum DpiMode {
		// Not DPI aware.
		dpiNone,

		// System DPI aware (i.e., checks at startup, but not afterwards).
		dpiSystem,

		// Per-monitor DPI aware (i.e., dynamically responds to changes).
		dpiMonitor,
	};

	// Current DPI mode.
	static DpiMode dpiMode;

	// Do we want to call "EnableNonClientScaling" (using the non-v2 mode of per monitor in Windows 8.1)?
	static bool callNonClientScaling;

	// Misc. Win32 functions we load. These might be null.
	typedef BOOL (WINAPI *EnableNonClientDpiScaling)(HWND);
	EnableNonClientDpiScaling enableNonClientDpiScaling;
	typedef UINT (WINAPI *GetDpiForSystem)();
	GetDpiForSystem getDpiForSystem;
	typedef UINT (WINAPI *GetDpiForWindow)(HWND);
	GetDpiForWindow getDpiForWindow;
	typedef int (WINAPI *GetSystemMetricsForDpi)(int, UINT);
	GetSystemMetricsForDpi getSystemMetricsForDpi;

	void setDpiAware() {
		HMODULE user32 = GetModuleHandle(L"user32.dll");
		dpiMode = dpiNone;
		callNonClientScaling = false;

		enableNonClientDpiScaling = (EnableNonClientDpiScaling)GetProcAddress(user32, "EnableNonClientDpiScaling");
		getDpiForSystem = (GetDpiForSystem)GetProcAddress(user32, "GetDpiForSystem");
		getDpiForWindow = (GetDpiForWindow)GetProcAddress(user32, "GetDpiForWindow");
		getSystemMetricsForDpi = (GetSystemMetricsForDpi)GetProcAddress(user32, "GetSystemMetricsForDpi");

		// Windows 10 and onwards (per monitor v2)
		typedef BOOL (WINAPI *SetDpiContext)(DWORD);
		SetDpiContext dpiContext = (SetDpiContext)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
		if (dpiContext && dpiContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
			dpiMode = dpiMonitor;
			return;
		}

		// Windows 8.1 and onwards. If we're not on Windows 10 (1607), this will leave menus and
		// window border unscaled (which the Microsoft samples do not deal with, so we won't either).
		{
			// Note: This is a known DLL, so this does not pose any danger of loading the wrong DLL.
			HMODULE shcore = LoadLibrary(L"SHCORE.DLL");

			typedef HRESULT (WINAPI *SetDpiProcess)(PROCESS_DPI_AWARENESS);
			SetDpiProcess dpiProcess = (SetDpiProcess)GetProcAddress(shcore, "SetProcessDpiAwareness");
			if (dpiProcess && dpiProcess(PROCESS_PER_MONITOR_DPI_AWARE) == S_OK) {
				dpiMode = dpiMonitor;
				callNonClientScaling = true;
			}

			FreeLibrary(shcore);

			if (dpiMode != dpiNone)
				return;
		}

		typedef void (WINAPI *SetDpiAware)();
		SetDpiAware dpiAware = (SetDpiAware)GetProcAddress(user32, "SetProcessDPIAware");
		if (dpiAware) {
			dpiAware();
			dpiMode = dpiSystem;
		}
	}

	void enableNcScaling(HWND hWnd) {
		if (!callNonClientScaling)
			return;

		if (enableNonClientDpiScaling)
			enableNonClientDpiScaling(hWnd);
	}

	Nat windowDpi(HWND hWnd) {
		if (dpiMode == dpiMonitor && getDpiForWindow) {
			return getDpiForWindow(hWnd);
		} else if (dpiMode == dpiSystem && getDpiForSystem) {
			return getDpiForSystem();
		} else {
			return defaultDpi;
		}
	}

	int dpiSystemMetrics(int index, Nat dpi) {
		if (getSystemMetricsForDpi)
			return getSystemMetricsForDpi(index, dpi);
		else
			return GetSystemMetrics(index);
	}

	Float dpiScale(Nat dpi) {
		return Float(dpi) / Float(defaultDpi);
	}

	Float dpiScaleInv(Nat dpi) {
		return Float(defaultDpi) / Float(dpi);
	}

	Point dpiToPx(Nat dpi, Point pt) {
		return pt * dpiScale(dpi);
	}

	Size dpiToPx(Nat dpi, Size size) {
		return size * dpiScale(dpi);
	}

	Rect dpiToPx(Nat dpi, Rect rect) {
		Float scale = dpiScale(dpi);
		rect.p0 *= scale;
		rect.p1 *= scale;
		return rect;
	}

	Point dpiFromPx(Nat dpi, Point pt) {
		return pt * dpiScaleInv(dpi);
	}

	Size dpiFromPx(Nat dpi, Size size) {
		return size * dpiScaleInv(dpi);
	}

	Rect dpiFromPx(Nat dpi, Rect rect) {
		Float scale = dpiScaleInv(dpi);
		rect.p0 *= scale;
		rect.p1 *= scale;
		return rect;
	}

}

#endif
