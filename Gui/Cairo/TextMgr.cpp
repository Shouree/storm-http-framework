#include "stdafx.h"
#include "TextMgr.h"
#include "Gui/PangoLayout.h"

#ifdef GUI_GTK

namespace gui {

	CairoText::CairoText() {
		// Note: This should be executed from the rendering thread. Otherwise, we get a shared font
		// map with the UI thread, which could lead to strange crashes.
		context = pango_font_map_create_context(pango_cairo_font_map_get_default());
	}

	CairoText::~CairoText() {
		g_object_unref(context);
	}

	CairoText::Resource CairoText::createFont(const Font *font) {
		// We're reusing the fonts already in the Font object.
		// In the future, we might want to create our own copies to avoid shared data.
		// It does, however, seem like the font description is a "dumb" container as
		// it does not require access to a context of any sort when creating it.
		return CairoText::Resource();
	}

	static void freeLayout(void *layout) {
		pango::free((PangoLayout *)layout);
	}

	CairoText::Resource CairoText::createLayout(const Text *text) {
		PangoLayout *l = pango::create(context, text);
		return Resource(l, &freeLayout);
	}

	bool CairoText::updateBorder(void *layout, Size s) {
		pango::updateBorder((PangoLayout *)layout, s);
		return true;
	}

	CairoText::EffectResult CairoText::addEffect(void *layout, const TextEffect &effect, Font *, Str *myText, Graphics *) {
		return pango::addEffect((PangoLayout *)layout, effect, myText);
	}

	Size CairoText::size(void *layout) {
		return pango::size((PangoLayout *)layout);
	}

	Array<TextLine *> *CairoText::lineInfo(void *layout, Text *t) {
		return pango::lineInfo((PangoLayout *)layout, t);
	}

	Array<Rect> *CairoText::boundsOf(void *layout, Text *text, Str::Iter begin, Str::Iter end) {
		return pango::boundsOf((PangoLayout *)layout, text, begin, end);
	}

}

#endif
