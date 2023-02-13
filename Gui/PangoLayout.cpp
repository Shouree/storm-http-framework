#include "stdafx.h"
#include "PangoLayout.h"
#include "Core/Convert.h"
#include "Core/Utf.h"
#include <limits>

#ifdef GUI_GTK

namespace gui {

	static inline Rect fromPango(const PangoRectangle &r) {
		return Rect(Point(fromPango(r.x), fromPango(r.y)), Size(fromPango(r.width), fromPango(r.height)));
	}


	namespace pango {

		static PangoAttrList *layout_attrs(PangoLayout *l) {
			PangoAttrList *attrs = pango_layout_get_attributes(l);
			if (!attrs) {
				attrs = pango_attr_list_new();
				pango_layout_set_attributes(l, attrs);
				pango_attr_list_unref(attrs);
			}
			return attrs;
		}

		static inline guint16 pangoColor(Float v) {
			return guint16(v * 65535);
		}

		static inline size_t byteOffset(Str::Iter storm, const char *pango, Str::Iter goal) {
			size_t offset = 0;
			while (storm != Str::Iter() && storm.offset() < goal.offset()) {
				// Advance each iterator one codepoint.
				++storm;

				// Skip the first utf-8 byte...
				offset++;
				// ...and skip any continuation bytes.
				while (utf8::isCont(pango[offset]))
					offset++;
			}

			return offset;
		}

		static inline bool inRange(int prev, int curr, int target) {
			if (curr == prev)
				return target == prev;
			else if (curr > prev)
				return target > prev && target <= curr;
			else
				return target >= curr && target < prev;
		}

		PangoLayout *create(PangoContext *context, const Text *text) {
			PangoLayout *l = pango_layout_new(context);

			Font *font = text->font();

			pango_layout_set_wrap(l, PANGO_WRAP_WORD_CHAR);
			pango_layout_set_font_description(l, font->desc());
			updateBorder(l, text->layoutBorder());
			const char *utf8 = text->text()->utf8_str();
			int utf8Bytes = strlen(utf8);
			pango_layout_set_text(l, utf8, utf8Bytes);

			Float tabWidth = text->font()->tabWidth();
			if (tabWidth > 0) {
				PangoTabArray *array = pango_tab_array_new(1, false);
				pango_tab_array_set_tab(array, 0, PANGO_TAB_LEFT, toPango(tabWidth));
				pango_layout_set_tabs(l, array);
				pango_tab_array_free(array);
			}

			if (font->underline()) {
				PangoAttrList *attrs = layout_attrs(l);
				PangoAttribute *attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
				attr->start_index = 0;
				attr->end_index = utf8Bytes;
				pango_attr_list_change(attrs, attr);
			}

			if (font->strikeOut()) {
				PangoAttrList *attrs = layout_attrs(l);
				PangoAttribute *attr = pango_attr_strikethrough_new(TRUE);
				attr->start_index = 0;
				attr->end_index = utf8Bytes;
				pango_attr_list_change(attrs, attr);
			}

			return l;
		}

		void free(PangoLayout *layout) {
			g_object_unref(layout);
		}

		void updateBorder(PangoLayout *l, Size s) {
			int w = -1;
			int h = -1;
			if (s.w < std::numeric_limits<float>::max())
				w = toPango(s.w);
			if (s.h < std::numeric_limits<float>::max())
				h = toPango(s.h);
			pango_layout_set_width(l, w);
			pango_layout_set_height(l, h);
		}

		TextMgr::EffectResult addEffect(PangoLayout *l, const TextEffect &effect, Str *myText) {
			PangoAttrList *attrs = layout_attrs(l);

			Str::Iter begin = myText->posIter(effect.from);
			Str::Iter end = myText->posIter(effect.to);

			const char *text = pango_layout_get_text(l);
			size_t beginBytes = byteOffset(myText->begin(), text, begin);
			size_t endBytes = beginBytes + byteOffset(begin, text + beginBytes, end);

			PangoAttribute *attr = null;

			switch (effect.type) {
			case TextEffect::tColor:
				attr = pango_attr_foreground_new(pangoColor(effect.d0), pangoColor(effect.d1), pangoColor(effect.d2));
				if (effect.d3 < 1.0f) {
					attr->start_index = beginBytes;
					attr->end_index = endBytes;
					pango_attr_list_change(attrs, attr);

					attr = pango_attr_foreground_alpha_new(pangoColor(effect.d3));
				}
				break;
			case TextEffect::tUnderline:
				// There is SINGLE_LINE as well.
				attr = pango_attr_underline_new(effect.boolean() ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE);
				break;
			case TextEffect::tStrikeOut:
				attr = pango_attr_strikethrough_new(effect.boolean());
				break;
			case TextEffect::tItalic:
				attr = pango_attr_style_new(effect.boolean() ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
				break;
			case TextEffect::tWeight:
				attr = pango_attr_weight_new(PangoWeight(effect.integer()));
				break;
			case TextEffect::tFamily:
				attr = pango_attr_family_new(effect.family()->utf8_str());
				break;
			case TextEffect::tScaleSize:
				attr = pango_attr_scale_new(effect.d0);
				break;
			default:
#ifdef DEBUG
				WARNING(L"Unknown effect type: " << TO_S(myText, effect));
#endif
				break;
			}

			if (attr) {
				attr->start_index = beginBytes;
				attr->end_index = endBytes;
				pango_attr_list_change(attrs, attr);

				// Tell Pango to re-compute the layout since we changed the formatting. Otherwise we are
				// not able to modify the text formatting after we have rendered it once.
				pango_layout_context_changed(l);
			}

			return TextMgr::eApplied;
		}

		Size size(PangoLayout *l) {
			int w, h;
			pango_layout_get_size(l, &w, &h);
			return Size(fromPango(w), fromPango(h));
		}

		Array<TextLine *> *lineInfo(PangoLayout *l, Text *t) {
			Array<TextLine *> *result = new (t) Array<TextLine *>();
			const char *text = pango_layout_get_text(l);

			PangoLayoutIter *iter = pango_layout_get_iter(l);
			do {
				Float baseline = fromPango(pango_layout_iter_get_baseline(iter));

				PangoLayoutLine *line = pango_layout_iter_get_line_readonly(iter);
				int from = line->start_index;
				Str *z = new (t) Str(toWChar(t->engine(), text + from, line->length));
				*result << new (t) TextLine(baseline, z);
			} while (pango_layout_iter_next_line(iter));
			pango_layout_iter_free(iter);

			return result;
		}

		Array<Rect> *boundsOf(PangoLayout *l, Text *stormText, Str::Iter begin, Str::Iter end) {
			Array<Rect> *result = new (stormText) Array<Rect>();
			const char *text = pango_layout_get_text(l);
			size_t beginBytes = byteOffset(stormText->text()->begin(), text, begin);
			size_t endBytes = beginBytes + byteOffset(begin, text + beginBytes, end);

			if (beginBytes == endBytes)
				return result;

			PangoRectangle rect;

			PangoLayoutIter *iter = pango_layout_get_iter(l);
			int lastIndex = pango_layout_iter_get_index(iter);
			do {
				int index = pango_layout_iter_get_index(iter);
				if (inRange(lastIndex, index, int(beginBytes)))
					break;

				lastIndex = index;
			} while (pango_layout_iter_next_cluster(iter));

			pango_layout_iter_get_cluster_extents(iter, NULL, &rect);
			Rect lineBounds = fromPango(rect);
			int lineStart = pango_layout_iter_get_line_readonly(iter)->start_index;

			while (pango_layout_iter_next_cluster(iter)) {
				int index = pango_layout_iter_get_index(iter);
				if (inRange(lastIndex, index, int(endBytes)))
					break;
				lastIndex = index;

				pango_layout_iter_get_cluster_extents(iter, NULL, &rect);
				int currLine = pango_layout_iter_get_line_readonly(iter)->start_index;

				// New line?
				if (currLine != lineStart) {
					*result << lineBounds;
					lineBounds = fromPango(rect);
					lineStart = currLine;
				} else {
					lineBounds = lineBounds.include(fromPango(rect));
				}
			}

			*result << lineBounds;

			pango_layout_iter_free(iter);

			return result;
		}

	}
}

#endif
