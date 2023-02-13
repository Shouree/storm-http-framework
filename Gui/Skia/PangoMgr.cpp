#include "stdafx.h"
#include "PangoMgr.h"
#include "PangoText.h"
#include "Gui/PangoLayout.h"

#ifdef GUI_ENABLE_SKIA

namespace gui {

	SkiaPangoMgr::SkiaPangoMgr() {
		// Note: This should be executed from the rendering thread. Otherwise, we get a shared font
		// map with the UI thread, which could lead to strange crashes.
		context = pango_font_map_create_context(pango_cairo_font_map_get_default());
	}

	SkiaPangoMgr::~SkiaPangoMgr() {
		g_object_unref(context);
	}

	SkiaPangoMgr::Resource SkiaPangoMgr::createFont(const Font *font) {
		// We're reusing the fonts already in the Font object.
		// In the future, we might want to create our own copies to avoid shared data.
		// It does, however, seem like the font description is a "dumb" container as
		// it does not require access to a context of any sort when creating it.
		return SkiaPangoMgr::Resource();
	}

	static void freeLayout(void *layout) {
		PangoText *text = (PangoText *)layout;
		delete text;
	}

	SkiaPangoMgr::Resource SkiaPangoMgr::createLayout(const Text *text) {
		PangoLayout *l = pango::create(context, text);
		return Resource(new PangoText(l, cache), &freeLayout);
	}

	bool SkiaPangoMgr::updateBorder(void *layout, Size s) {
		PangoText *t = (PangoText *)layout;
		pango::updateBorder(t->pango, s);
		t->invalidate();
		return true;
	}

	SkiaPangoMgr::EffectResult SkiaPangoMgr::addEffect(void *layout, const TextEffect &effect, Font *, Str *myText, Graphics *) {
		PangoText *t = (PangoText *)layout;
		EffectResult r = pango::addEffect(t->pango, effect, myText);

		t->invalidate();
		return r;
	}

	Size SkiaPangoMgr::size(void *layout) {
		PangoText *t = (PangoText *)layout;
		return pango::size(t->pango);
	}

	Array<TextLine *> *SkiaPangoMgr::lineInfo(void *layout, Text *text) {
		PangoText *t = (PangoText *)layout;
		return pango::lineInfo(t->pango, text);
	}

	Array<Rect> *SkiaPangoMgr::boundsOf(void *layout, Text *text, Str::Iter begin, Str::Iter end) {
		PangoText *t = (PangoText *)layout;
		return pango::boundsOf(t->pango, text, begin, end);
	}

}

#endif
