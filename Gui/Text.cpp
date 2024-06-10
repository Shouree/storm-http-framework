#include "stdafx.h"
#include "Text.h"
#include "RenderMgr.h"
#include "TextMgr.h"
#include <limits>

namespace gui {

	/**
	 * Text line.
	 */

	TextLine::TextLine(Float baseline, Str *text) :
		baseline(baseline), text(text) {}

	void TextLine::toS(StrBuf *to) const {
		*to << S("{baseline=") << baseline << S(" text=")
			<< text << S("}");
	}

	/**
	 * Text effect.
	 */

	TextEffect::TextEffect() : type(tNone), from(0), to(0), d0(0), d1(0), d2(0), d3(0), ptr(null) {}

	TextEffect::TextEffect(Type type, Str::Iter begin, Str::Iter end, Float d0, Float d1, Float d2, Float d3, void *ptr)
		: type(type), from(begin.offset()), to(end.offset()),
		  d0(d0), d1(d1), d2(d2), d3(d3), ptr(ptr) {}

	TextEffect TextEffect::color(Str::Iter begin, Str::Iter end, Color color) {
		return TextEffect(tColor, begin, end, color.r, color.g, color.b, color.a, null);
	}

	TextEffect TextEffect::underline(Str::Iter begin, Str::Iter end, Bool enable) {
		return TextEffect(tUnderline, begin, end, enable ? 1.0f : 0.0f, 0, 0, 0, null);
	}

	TextEffect TextEffect::strikeOut(Str::Iter begin, Str::Iter end, Bool enable) {
		return TextEffect(tStrikeOut, begin, end, enable ? 1.0f : 0.0f, 0, 0, 0, null);
	}

	TextEffect TextEffect::italic(Str::Iter begin, Str::Iter end, Bool enable) {
		return TextEffect(tItalic, begin, end, enable ? 1.0f : 0.0f, 0, 0, 0, null);
	}

	TextEffect TextEffect::weight(Str::Iter begin, Str::Iter end, Int weight) {
		return TextEffect(tWeight, begin, end, Float(weight), 0, 0, 0, null);
	}

	TextEffect TextEffect::family(Str::Iter begin, Str::Iter end, Str *family) {
		return TextEffect(tFamily, begin, end, 0, 0, 0, 0, family);
	}

	TextEffect TextEffect::scaleSize(Str::Iter begin, Str::Iter end, Float size) {
		return TextEffect(tScaleSize, begin, end, size, 0, 0, 0, null);
	}

	void TextEffect::toS(StrBuf *to) const {
		*to << from << S("-") << to << S(" ");

		switch (type) {
		case TextEffect::tNone:
			*to << S("<no effect>");
			break;
		case TextEffect::tColor:
			*to << S("color: ") << color();
			break;
		case TextEffect::tUnderline:
			*to << S("underline: ") << boolean();
			break;
		case TextEffect::tStrikeOut:
			*to << S("strike out: ") << boolean();
			break;
		case TextEffect::tItalic:
			*to << S("italic: ") << boolean();
			break;
		case TextEffect::tWeight:
			*to << S("weight: ") << integer();
			break;
		case TextEffect::tFamily:
			*to << S("family: ") << family();
			break;
		case TextEffect::tScaleSize:
			*to << S("scale size: ") << d0;
			break;
		default:
			*to << S("<unknown effect>");
			break;
		}
	}

	/**
	 * The text object itself.
	 */

	// Small extra space added around the border to prevent unneeded wrapping.
	static const float borderExtra = 0.001f;

	Text::Text(Str *text, Font *font)
		: myText(text), myFont(font), myEffects(null), appliedEffects(0) {

		float maxFloat = std::numeric_limits<float>::max();
		myBorder = Size(maxFloat, maxFloat);
		init();
	}

	Text::Text(Str *text, Font *font, Size size)
		: myText(text), myFont(font), myBorder(size), myEffects(null), appliedEffects(0) {

		init();
	}

	void Text::init() {
		mgr = renderMgr(engine())->text();
		TextMgr::Resource r = mgr->createLayout(this);
		layout = r.data;
		cleanup = r.cleanup;
	}

	void Text::recreate() {
		// Apply any outstanding effects now.
		if (myEffects && appliedEffects < myEffects->count()) {
			while (appliedEffects < myEffects->count()) {
				TextEffect effect = myEffects->at(appliedEffects);
				myEffects->at(appliedEffects) = TextEffect();
				appliedEffects++;
				insertEffect(effect);
			}
			cleanupEffects();
		}

		if (cleanup)
			(*cleanup)(layout);
		TextMgr::Resource r = mgr->createLayout(this);
		layout = r.data;
		cleanup = r.cleanup;
	}

	Text::~Text() {
		if (layout && cleanup) {
			(*cleanup)(layout);
		}
	}

	Size Text::size() {
		return mgr->size(layout) + Size(borderExtra, borderExtra);
	}

	void Text::layoutBorder(Size border) {
		if (!mgr->updateBorder(layout, border)) {
			recreate();
		}
	}

	void Text::effect(TextEffect effect) {
		if (!myEffects)
			myEffects = new (this) Array<TextEffect>();

		// Try to apply it now.
		TextMgr::EffectResult r = mgr->addEffect(layout, effect, myFont, myText, null);
		if (r == TextMgr::eApplied) {
			// Applied now, update!
			insertEffect(effect);
		} else {
			// Apply it later.
			*myEffects << effect;

			if (r == TextMgr::eReCreate) {
				// We could be a bit more eager in this case. That would be beneficial if (for some
				// reason) the effect had an impact on size or layout computations.
			}
		}
	}

	void Text::insertEffect(TextEffect effect) {
		if (effect.empty())
			return;

		for (Nat i = 0; i < appliedEffects; i++) {
			TextEffect &e = myEffects->at(i);

			// Only examine effects of the same type.
			if (e.type != effect.type) {
				continue;
			}

			// Exactly the same range?
			if (e.from == effect.from && e.to == effect.to) {
				e = effect;
				// Done!
				return;
			}

			// Is the existing one smaller than this one?
			if (e.from >= effect.from && e.to <= effect.to) {
				removeEffectI(i); i--;
				continue;
			}

			// The other one before this one?
			if (e.to >= effect.from && e.to < effect.from) {
				// Merge them?
				if (e.sameData(effect)) {
					effect.from = e.from;
					removeEffectI(i); i--;
				} else {
					// Shorten the other one.
					e.to = effect.from;
				}
				continue;
			}

			// The other one after this one?
			if (e.from > effect.from && e.from <= effect.to) {
				// Merge them?
				if (e.sameData(effect)) {
					effect.to = e.to;
					removeEffectI(i); i--;
				} else {
					// Shorten the other one.
					e.from = effect.to;
				}
				continue;
			}

			// The existing one is larger than this one.
			if (e.from < effect.from && e.to > effect.to) {
				if (e.sameData(effect)) {
					// Nothing to do, it is the same data.
					return;
				} else {
					// Split the other one.
					TextEffect last = e;
					e.to = effect.from;
					last.from = effect.to;

					insertEffectI(last);
				}
			}
		}

		insertEffectI(effect);
	}

	void Text::removeEffectI(Nat id) {
		// Move effects "forward" until we have a "hole" right before "appliedEffects".
		for (Nat i = id; i + 1 < appliedEffects; i++) {
			myEffects->at(i) = myEffects->at(i + 1);
		}

		// Create a proper "hole".
		myEffects->at(appliedEffects - 1) = TextEffect();
	}

	void Text::insertEffectI(TextEffect effect) {
		// Go from the end and backwards.
		Nat empty = appliedEffects;
		for (Nat i = appliedEffects; i > 0; i--) {
			if (myEffects->at(i - 1).any())
				break;
			empty = i - 1;
		}

		// Full?
		if (empty >= appliedEffects) {
			// No room. Insert it, moving the others one step ahead.
			myEffects->insert(appliedEffects, effect);
			appliedEffects++;
		} else {
			// Replace 'empty'?
			myEffects->at(empty) = effect;
		}
	}

	void Text::cleanupEffects() {
		while (appliedEffects < myEffects->count()) {
			if (myEffects->last().type == TextEffect::tNone)
				myEffects->pop();
			else
				break;
		}
	}

	Array<TextLine *> *Text::lineInfo() {
		return mgr->lineInfo(layout, this);
	}

	Array<Rect> *Text::boundsOf(Str::Iter begin, Str::Iter end) {
		return mgr->boundsOf(layout, this, begin, end);
	}

	Array<TextEffect> *Text::effects() const {
		if (!myEffects) {
			return new (this) Array<TextEffect>();
		} else {
			return new (this) Array<TextEffect>(*myEffects);
		}
	}

	void *Text::backendLayout(Graphics *g) {
		if (myEffects && appliedEffects < myEffects->count()) {

			while (appliedEffects < myEffects->count()) {
				TextMgr::EffectResult r = mgr->addEffect(layout, myEffects->at(appliedEffects), myFont, myText, g);
				if (r == TextMgr::eApplied) {
					// Clear it out and merge it back in.
					TextEffect effect = myEffects->at(appliedEffects);
					myEffects->at(appliedEffects) = TextEffect();
					appliedEffects++;
					insertEffect(effect);
				} else if (r == TextMgr::eReCreate) {
					// We need to recreate the entire thing.
					recreate();
					break;
				} else if (r == TextMgr::eWait) {
					// Wrong context apparently. Wait for other draws.
					break;
				}
			}

			cleanupEffects();
		}

		// Now, we are ready!
		return layout;
	}

}
