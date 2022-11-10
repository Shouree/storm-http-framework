#pragma once

#ifdef GUI_WIN32

namespace gui {

	/**
	 * Functions for DPI-aware GUI:s.
	 *
	 * These are dynamically loaded from user32.dll so that we are somewhat backwards compatible with
	 * earlier versions of Windows (for example, Windows 7 did not fully support everything here).
	 *
	 * We support global DPI awareness on Windows Vista and onwards, and per-monitor DPI awareness
	 * on Windows 10 (1607) and onwards.
	 */

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

	// Initialize DPI awareness. Other functions here might not work unless this is called first.
	void setDpiAware();

	// Call to enable non-client scaling for a newly created window.
	void enableNcScaling(HWND hWnd);

	// Get the current DPI for a window.
	Nat windowDpi(HWND hWnd);

	// Get system metrics for DPI.
	int dpiSystemMetrics(int index, Nat dpi);

	// Get system parameters for DPI.
	BOOL dpiSystemParametersInfo(UINT action, UINT param, void *data, UINT winIni, UINT dpi);

	// AdjustWindowRectEx
	BOOL dpiAdjustWindowRectEx(RECT *rect, DWORD style, bool menu, DWORD exStyle, Nat dpi);

	// Compute a scaling factor for the DPI.
	Float dpiScale(Nat dpi);
	Float dpiScaleInv(Nat dpi);

	// Scale various types according to DPI.
	Point dpiToPx(Nat dpi, Point pt);
	Size dpiToPx(Nat dpi, Size size);
	Rect dpiToPx(Nat dpi, Rect rect);

	Point dpiFromPx(Nat dpi, Point pt);
	Size dpiFromPx(Nat dpi, Size size);
	Rect dpiFromPx(Nat dpi, Rect rect);

}

#endif

namespace gui {
	extern const Nat defaultDpi;
}
