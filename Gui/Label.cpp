#include "stdafx.h"
#include "Label.h"
#include "Container.h"
#include "Win32Dpi.h"

namespace gui {

	Label::Label(Str *title) : hAlign(hLeft), vAlign(vTop) {
		text(title);
	}

	Label::Label(Str *title, HAlign halign) : hAlign(halign), vAlign(vTop) {
		text(title);
	}

	Label::Label(Str *title, VAlign valign) : hAlign(hLeft), vAlign(valign) {
		text(title);
	}

	Label::Label(Str *title, HAlign halign, VAlign valign) : hAlign(halign), vAlign(valign) {
		text(title);
	}

#ifdef GUI_WIN32

	bool Label::create(ContainerBase *parent, nat id) {
		DWORD flags = childFlags;
		switch (hAlign) {
		case hLeft:
			flags |= SS_LEFT;
			break;
		case hCenter:
			flags |= SS_CENTER;
			break;
		case hRight:
			flags |= SS_RIGHT;
			break;
		}
		switch (vAlign) {
		case vTop:
			// No flag.
			break;
		case vCenter:
			flags |= SS_CENTERIMAGE;
			break;
		case vBottom:
			// Actually not supported...
			break;
		}
		flags |= SS_NOPREFIX; // Don't treat & characters specially.
		return createEx(WC_STATIC, flags, 0, parent->handle().hwnd(), id);
	}

	Size Label::minSize() {
		Str *s = text();
		Bool hasNewline = false;
		for (Str::Iter i = s->begin(), e = s->end(); i != e; ++i) {
			if (i.v() == Char('\n')) {
				hasNewline = true;
				break;
			}
		}

		if (hasNewline) {
			Nat dpi = currentDpi();
			// Text layout does not always scale linearly.
			return dpiFromPx(dpi, font()->stringSize(text(), dpi));
		} else {
			// If no newlines, the difference is small enough to ignore. It causes issues in the layout.
			return font()->stringSize(text());
		}
	}

#endif
#ifdef GUI_GTK

	bool Label::create(ContainerBase *parent, nat id) {
		GtkWidget *label = gtk_label_new(text()->utf8_str());

		switch (hAlign) {
		case hLeft:
			gtk_widget_set_halign(label, (GtkAlign)GTK_ALIGN_START);
			break;
		case hCenter:
			gtk_widget_set_halign(label, (GtkAlign)GTK_ALIGN_CENTER);
			break;
		case hRight:
			gtk_widget_set_halign(label, (GtkAlign)GTK_ALIGN_END);
			break;
		}

		switch (vAlign) {
		case vTop:
			gtk_widget_set_valign(label, (GtkAlign)GTK_ALIGN_START);
			break;
		case vCenter:
			gtk_widget_set_valign(label, (GtkAlign)GTK_ALIGN_CENTER);
			break;
		case vBottom:
			gtk_widget_set_valign(label, (GtkAlign)GTK_ALIGN_END);
			break;
		}

		// GValue zero = G_VALUE_INIT;
		// g_value_init(&zero, G_TYPE_FLOAT);
		// g_value_set_float(&zero, (gfloat)0.0f);
		// g_object_set_property(G_OBJECT(label), "xalign", &zero);
		// g_object_set_property(G_OBJECT(label), "yalign", &zero);
		// g_value_unset(&zero);

		initWidget(parent, label);
		return true;
	}

	void Label::text(Str *text) {
		if (created()) {
			gtk_label_set_text(GTK_LABEL(handle().widget()), text->utf8_str());
		}
		Window::text(text);
	}

	Size Label::minSize() {
		gint w = 0, h = 0;

		// The win32 implementation will give a size of zero for empty strings.
		if (created() && text()->any()) {
			gtk_widget_get_preferred_width(handle().widget(), &w, NULL);
			gtk_widget_get_preferred_height(handle().widget(), &h, NULL);
		}

		return Size(Float(w), Float(h));
	}

#endif
}
