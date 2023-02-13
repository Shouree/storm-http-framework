#pragma once

#include "Gui/TextMgr.h"

#ifdef GUI_GTK

namespace gui {

	class CairoText : public TextMgr {
	public:
		// Create.
		CairoText();

		// Destroy.
		~CairoText();

		// Create a font.
		virtual Resource createFont(const Font *font);

		// Create a text layout.
		virtual Resource createLayout(const Text *text);

		// Update the layout border of a layout.
		virtual bool updateBorder(void *layout, Size border);

		// Add a new effect to the layout. Note: "graphics" may be null if we are not currently rendering.
		virtual EffectResult addEffect(void *layout, const TextEffect &effect, Font *font, Str *text, MAYBE(Graphics *) graphics);

		// Get the actual size of the layout.
		virtual Size size(void *layout);

		// Get information about each line of the formatted text.
		virtual Array<TextLine *> *lineInfo(void *layout, Text *text);

		// Get a set of rectangles that cover a range of characters.
		virtual Array<Rect> *boundsOf(void *layout, Text *text, Str::Iter begin, Str::Iter end);

	private:
		// Pango context.
		PangoContext *context;
	};

}

#endif
