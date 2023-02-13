#include "stdafx.h"
#include "TextMgr.h"
#include "Exception.h"
#include "Graphics.h"
#include "Device.h"

#ifdef GUI_WIN32

namespace gui {

	static void check(HRESULT r, const wchar *msg) {
		if (FAILED(r)) {
			Engine &e = runtime::someEngine();
			Str *m = TO_S(e, msg << ::toS(r).c_str());
			throw new (e) GuiError(m);
		}
	}

	D2DText::D2DText() {
		DWRITE_FACTORY_TYPE type = DWRITE_FACTORY_TYPE_SHARED;
		HRESULT r = DWriteCreateFactory(type, __uuidof(IDWriteFactory), (IUnknown **)&factory.v);
		check(r, S("Failed to intitialize Direct Write: "));
	}

	static void destroyCOM(void *object) {
		if (object) {
			IUnknown *i = (IUnknown *)object;
			i->Release();
		}
	}

	D2DText::Resource D2DText::createFont(const Font *font) {
		IDWriteTextFormat *out = null;
		DWRITE_FONT_STYLE style = font->italic() ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
		DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;
		HRESULT r = factory->CreateTextFormat(font->name()->c_str(),
											NULL,
											(DWRITE_FONT_WEIGHT)font->weight(),
											style,
											stretch,
											font->pxHeight(),
											L"", // locale, we use defaults. Was "en-us".
											&out);

		check(r, S("Failed to create a Direct Write font."));

		if (font->tabWidth() > 0)
			out->SetIncrementalTabStop(font->tabWidth());

		return Resource(out, &destroyCOM);
	}

	D2DText::Resource D2DText::createLayout(const Text *text) {
		Font *font = text->font();
		Str *t = text->text();
		Size border = text->layoutBorder();
		IDWriteTextFormat *format = (IDWriteTextFormat *)font->backendFont();
		IDWriteTextLayout *layout;
		HRESULT r = factory->CreateTextLayout(t->c_str(), t->peekLength(), format, border.w, border.h, &layout);
		check(r, S("Failed to create a text layout: "));

		// We need to set these if they are present in the font...
		DWRITE_TEXT_RANGE range = { 0, t->peekLength() };
		if (font->underline()) {
			layout->SetUnderline(TRUE, range);
		}

		if (font->strikeOut()) {
			layout->SetStrikethrough(TRUE, range);
		}

		return D2DText::Resource(layout, &destroyCOM);
	}

	bool D2DText::updateBorder(void *layout, Size border) {
		IDWriteTextLayout *l = (IDWriteTextLayout *)layout;
		l->SetMaxWidth(border.w);
		l->SetMaxHeight(border.h);
		return true;
	}

	D2DText::EffectResult D2DText::addEffect(void *layout, const TextEffect &effect, Font *font, Str *text, Graphics *graphics) {

		IDWriteTextLayout *l = (IDWriteTextLayout *)layout;
		DWRITE_TEXT_RANGE range = { effect.from, effect.to - effect.from };

		switch (effect.type) {
		case TextEffect::tColor:
		// It should be a D2DGraphics object.
			if (D2DGraphics *g = as<D2DGraphics>(graphics)) {
				ID2D1SolidColorBrush *brush;
				g->getSurface().target()->CreateSolidColorBrush(dx(effect.color()), &brush);
				l->SetDrawingEffect(brush, range);
				brush->Release();
			} else {
				// We need to apply these later.
				return eWait;
			}
			break;
		case TextEffect::tUnderline:
			l->SetUnderline(effect.boolean() ? TRUE : FALSE, range);
			break;
		case TextEffect::tStrikeOut:
			l->SetStrikethrough(effect.boolean() ? TRUE : FALSE, range);
			break;
		case TextEffect::tItalic:
			l->SetFontStyle(effect.boolean() ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, range);
			break;
		case TextEffect::tWeight:
			l->SetFontWeight(DWRITE_FONT_WEIGHT(effect.integer()), range);
			break;
		case TextEffect::tFamily:
			l->SetFontFamilyName(effect.family()->c_str(), range);
			break;
		case TextEffect::tScaleSize:
			l->SetFontSize(effect.d0 * font->pxHeight(), range);
			break;
		default:
#ifdef DEBUG
			WARNING(L"Unknown effect: " << TO_S(text, effect));
#endif
			break;
		}


		return eApplied;
	}

	Size D2DText::size(void *layout) {
		IDWriteTextLayout *l = (IDWriteTextLayout *)layout;
		DWRITE_TEXT_METRICS metrics;
		l->GetMetrics(&metrics);
		return Size(metrics.width, metrics.height);
	}

	Array<TextLine *> *D2DText::lineInfo(void *layout, Text *text) {
		IDWriteTextLayout *l = (IDWriteTextLayout *)layout;
		Str *myText = text->text();

		UINT32 numLines = 0;
		l->GetLineMetrics(NULL, 0, &numLines);

		DWRITE_LINE_METRICS *lines = new DWRITE_LINE_METRICS[numLines];
		l->GetLineMetrics(lines, numLines, &numLines);

		Array<TextLine *> *result = new (text) Array<TextLine *>();
		Nat pos = 0;
		Float yPos = 0;

		for (Nat i = 0; i < numLines; i++) {
			// The offsets in here seem to refer back to the original string (UTF-16 encoded) rather
			// than codepoint indices.
			DWRITE_LINE_METRICS &l = lines[i];

			Str::Iter start = myText->posIter(pos);
			Str::Iter end = myText->posIter(pos + l.length - l.newlineLength);

			*result << new (text) TextLine(yPos + l.baseline, myText->substr(start, end));
			yPos += l.height;
			pos += l.length;
		}

		delete []lines;

		return result;
	}

	static inline Rect computeRect(IDWriteTextLayout *l, Nat from, Nat to) {
		Rect total;
		DWRITE_HIT_TEST_METRICS metrics;
		for (Nat i = from; i < to; i += metrics.length) {
			FLOAT x, y;
			l->HitTestTextPosition(i, false, &x, &y, &metrics);

			Rect ch(Point(metrics.left, metrics.top), Size(metrics.width, metrics.height));
			if (i == from)
				total = ch;
			else
				total = total.include(ch);
		}

		return total;
	}

	Array<Rect> *D2DText::boundsOf(void *layout, Text *text, Str::Iter begin, Str::Iter end) {
		IDWriteTextLayout *l = (IDWriteTextLayout *)layout;
		Str *myText = text->text();

		Array<Rect> *result = new (text) Array<Rect>();

		UINT32 numLines = 0;
		l->GetLineMetrics(NULL, 0, &numLines);

		DWRITE_LINE_METRICS *lines = new DWRITE_LINE_METRICS[numLines];
		l->GetLineMetrics(lines, numLines, &numLines);

		Nat lineBegin = 0;
		for (Nat line = 0; line < numLines; line++) {
			Nat lineEnd = lineBegin + lines[line].length;

			if (lineEnd > begin.offset())
				*result << computeRect(l, max(lineBegin, begin.offset()), min(lineEnd, end.offset()));

			// Done?
			if (end.offset() <= lineEnd)
				break;

			lineBegin = lineEnd;
		}

		delete [] lines;

		return result;
	}

}

#endif
