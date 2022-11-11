#include "stdafx.h"
#include "Defaults.h"
#include "Win32Dpi.h"

namespace gui {

#ifdef GUI_WIN32

	static Font *defaultFont(EnginePtr e) {
		NONCLIENTMETRICS ncm;
		ncm.cbSize = sizeof(ncm);

		BOOL ok = dpiSystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0, defaultDpi);
		if (!ok) {
			// If it failed, try without the iPaddedBorderWidth, as that seems to be new for some
			// versions of the API.
			ncm.cbSize = sizeof(ncm) - sizeof(ncm.iPaddedBorderWidth);
			ok = dpiSystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0, defaultDpi);
		}

		if (!ok) {
			// Fall back to the plain version of the system call in case the DPI aware version does
			// not work as we expect for some reason.
			ncm.cbSize = sizeof(ncm) - sizeof(ncm.iPaddedBorderWidth);
			ok = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		}

		if (!ok) {
			WARNING(L"Failed to get the default system font.");
			LOGFONT &f = ncm.lfMessageFont;
			zeroMem(f);
			// These defaults are based on some sane defaults for Windows XP.
			f.lfHeight = -12;
			f.lfWeight = 400;
			wcscpy_s(f.lfFaceName, LF_FACESIZE, L"Segoe UI");
		}

		return new (e.v) Font(ncm.lfMessageFont);
	}

	Defaults sysDefaults(EnginePtr e) {
		Defaults d = {
			defaultFont(e),
			color(GetSysColor(COLOR_BTNFACE)), // Same as COLOR_3DFACE, which is for dialogs etc.
			color(GetSysColor(COLOR_BTNTEXT)),
		};
		return d;
	}

#endif
#ifdef GUI_GTK

	static Font *defaultFont(EnginePtr e, GtkStyleContext *style) {
		const PangoFontDescription *desc = null;
		gtk_style_context_get(style, GTK_STATE_FLAG_NORMAL, "font", &desc, NULL);
		return new (e.v) Font(*desc);
	}

	static Color backgroundColor(EnginePtr e, GtkStyleContext *style) {
		cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 1, 1);
		cairo_t *cairo = cairo_create(surface);

		cairo_set_source_rgb(cairo, 0, 1, 0);
		cairo_paint(cairo);

		gtk_render_background(style, cairo, 0, 0, 1, 1);

		cairo_surface_flush(surface);
		cairo_destroy(cairo);

		uint32_t data = *(uint32_t *)cairo_image_surface_get_data(surface);
		uint32_t r = (data >> 16) & 0xFF;
		uint32_t g = (data >>  8) & 0xFF;
		uint32_t b = (data >>  0) & 0xFF;
		cairo_surface_destroy(surface);

		return Color(byte(r), byte(g), byte(b));
	}

	static Color foregroundColor(EnginePtr e, GtkStyleContext *style) {
		GdkRGBA color;
		gtk_style_context_get_color(style, GTK_STATE_FLAG_NORMAL, &color);

		return Color(Float(color.red), Float(color.green), Float(color.blue), Float(color.alpha));
	}

	Defaults sysDefaults(EnginePtr e) {
		GtkWidget *dummyLabel = gtk_label_new("dummy");
		GtkWidget *dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_container_add(GTK_CONTAINER(dummyWindow), dummyLabel);

		GtkStyleContext *labelStyle = gtk_widget_get_style_context(dummyLabel);
		GtkStyleContext *windowStyle = gtk_widget_get_style_context(dummyWindow);
		Defaults d = {
			defaultFont(e, labelStyle),
			backgroundColor(e, windowStyle),
			foregroundColor(e, labelStyle),
		};
		gtk_widget_destroy(dummyLabel);
		gtk_widget_destroy(dummyWindow);
		return d;
	}

#endif

}
