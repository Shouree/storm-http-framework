#pragma once

#ifdef GUI_GTK

// Needed to get the definition of fc-font on some systems...
#define PANGO_ENABLE_BACKEND

#include <poll.h>
#include <X11/Xlib.h>
#define GL_GLEXT_PROTOTYPES
#include <cairo/cairo.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <gdk/gdkwayland.h>
#include "Cairo/CairoGL.h"
#include "GtkLayout.h"

// Some headers define 'Bool' for us. That is not good...
#undef Bool

namespace gui {

	class TextureContext;

	// More convenient matrix operations.
	cairo_matrix_t cairoMultiply(const cairo_matrix_t &a, const cairo_matrix_t &b);
	cairo_matrix_t cairo(Transform *tfm);

	// Convert to/from Pango units.
	inline int toPango(float val) { return int(ceilf(val * PANGO_SCALE)); }
	inline float fromPango(int val) { return float(val) / float(PANGO_SCALE); }
}

#else

// Allow using some types in class declarations even if we're not using Gtk+.
// TODO: Can we remove some of these? After rewriting the rendering, probably yes.
struct PangoContext;
struct GdkWindow;
struct GdkDisplay;
struct GtkListStore;

#endif
