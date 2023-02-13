#pragma once
#include "Font.h"
#include "Brush.h"
#include "Core/TObject.h"
#include "Core/Array.h"

namespace gui {

	class TextMgr;

	/**
	 * Information about a single line of formatted text.
	 */
	class TextLine : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR TextLine(Float baseline, Str *text);

		// The distance from the top of the line to the baseline.
		Float baseline;

		// Contents of the line.
		Str *text;

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

	/**
	 * Text effect. Used with the Text class.
	 */
	class TextEffect {
		STORM_VALUE;
	public:
		// Create an empty text effect.
		STORM_CTOR TextEffect();

		// Type of the effect
		enum Type {
			// No effect. The uninitialized state.
			STORM_NAME(tNone, none),

			// Set the text color. The four floats represent r, g, b and a respectively.
			STORM_NAME(tColor, color),

			// Set the underline. The data d0 is either 1 or 0.
			STORM_NAME(tUnderline, underline),

			// Set strike out. The data d0 is either 1 or 0.
			STORM_NAME(tStrikeOut, strikeOut),

			// Set italics. The data d0 is either 1 or 0.
			STORM_NAME(tItalic, italic),

			// Set weight. The data d0 is the weight.
			STORM_NAME(tWeight, weight),

			// Set font family. 'ptr' contains the name.
			STORM_NAME(tFamily, family),

			// Set font size. 'd0' contains the size.
			STORM_NAME(tScaleSize, scaleSize),
		};

		// Effect type.
		Type type;

		// String offset (int UTF-16 codepoints). Use 'begin' and 'end' in Storm to access these.
		Nat from;
		Nat to;

		// Get iterators into a string.
		inline Str::Iter STORM_FN begin(Str *s) const { return s->posIter(from); }
		inline Str::Iter STORM_FN end(Str *s) const { return s->posIter(to); }

		// Data. Depends on 'type'.
		Float d0;
		Float d1;
		Float d2;
		Float d3;

		// Pointer data.
		UNKNOWN(PTR_GC) void *ptr;

		// Is this effect empty?
		inline Bool STORM_FN empty() const { return type == tNone; }
		inline Bool STORM_FN any() const { return !empty(); }

		// Helper to get the font family.
		inline MAYBE(Str *) STORM_FN family() const {
			if (type == tFamily)
				return (Str *)ptr;
			else
				return null;
		}

		// Helper to get a color.
		inline Color STORM_FN color() const {
			return Color(d0, d1, d2, d3);
		}

		// Helper to get a bool.
		inline Bool STORM_FN boolean() const {
			return d0 > 0.1f;
		}

		// Helper to get an int.
		inline Int STORM_FN integer() const {
			return int(d0);
		}

		// Compare the data in here.
		inline bool sameData(const TextEffect &other) const {
			return d0 == other.d0
				&& d1 == other.d1
				&& d2 == other.d2
				&& d3 == other.d3;
		}

		// Create effects:
		static TextEffect STORM_FN color(Str::Iter begin, Str::Iter end, Color color);
		static TextEffect STORM_FN underline(Str::Iter begin, Str::Iter end, Bool enable);
		static TextEffect STORM_FN strikeOut(Str::Iter begin, Str::Iter end, Bool enable);
		static TextEffect STORM_FN italic(Str::Iter begin, Str::Iter end, Bool enable);
		static TextEffect STORM_FN weight(Str::Iter begin, Str::Iter end, Int weight);
		static TextEffect STORM_FN family(Str::Iter begin, Str::Iter end, Str *family);
		static TextEffect STORM_FN scaleSize(Str::Iter begin, Str::Iter end, Float size);

	private:
		// Constructor for all factory methods.
		TextEffect(Type type, Str::Iter begin, Str::Iter end, Float d0, Float d1, Float d2, Float d3, void *ptr);
	};

	// Output an effect.
	StrBuf &STORM_FN operator <<(StrBuf &to, const TextEffect &effect);

	/**
	 * Pre-formatted text prepared for rendering.
	 */
	class Text : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Create a single line of text.
		STORM_CTOR Text(Str *text, Font *font);

		// Create text that fits inside a square 'size' units big.
		STORM_CTOR Text(Str *text, Font *font, Size size);

		// Destroy.
		virtual ~Text();

		// Get the text in here.
		inline Str *STORM_FN text() const { return myText; }

		// Get the font used for this object.
		inline Font *STORM_FN font() const { return myFont; }

		// Size of the text inside the layout.
		Size STORM_FN size();

		// Layout border size. If no layout border is set from the constructor, it will return the
		// largest possible float value.
		inline Size STORM_FN layoutBorder() const { return myBorder; }
		void STORM_ASSIGN layoutBorder(Size size);

		// Get information about each line of the formatted text.
		Array<TextLine *> *STORM_FN lineInfo();

		// Get a set of rectangles that cover a range of characters.
		Array<Rect> *STORM_FN boundsOf(Str::Iter begin, Str::Iter end);

		// Add an effect.
		void STORM_FN effect(TextEffect effect);

		// Set the color of a particular range of characters.
		inline void STORM_FN color(Str::Iter begin, Str::Iter end, Color color) {
			effect(TextEffect::color(begin, end, color));
		}
		inline void STORM_FN color(Str::Iter begin, Str::Iter end, SolidBrush *color) {
			effect(TextEffect::color(begin, end, color->color()));
		}

		// Set underline on a particular range of characters.
		inline void STORM_FN underline(Str::Iter begin, Str::Iter end) {
			effect(TextEffect::underline(begin, end, true));
		}
		inline void STORM_FN underline(Str::Iter begin, Str::Iter end, Bool enable) {
			effect(TextEffect::underline(begin, end, enable));
		}

		// Set strike out on a particular range of characters.
		inline void STORM_FN strikeOut(Str::Iter begin, Str::Iter end) {
			effect(TextEffect::strikeOut(begin, end, true));
		}
		inline void STORM_FN strikeOut(Str::Iter begin, Str::Iter end, Bool enable) {
			effect(TextEffect::strikeOut(begin, end, enable));
		}

		// Enable italic style.
		inline void STORM_FN italic(Str::Iter begin, Str::Iter end) {
			effect(TextEffect::italic(begin, end, true));
		}
		inline void STORM_FN italic(Str::Iter begin, Str::Iter end, Bool enable) {
			effect(TextEffect::italic(begin, end, enable));
		}

		// Set font weight.
		inline void STORM_FN weight(Str::Iter begin, Str::Iter end, Int weight) {
			effect(TextEffect::weight(begin, end, weight));
		}

		// Set font family.
		inline void STORM_FN family(Str::Iter begin, Str::Iter end, Str *family) {
			effect(TextEffect::family(begin, end, family));
		}

		// Set font size. The size is expressed as a scale factor of the original font.
		inline void STORM_FN scaleSize(Str::Iter begin, Str::Iter end, Float size) {
			effect(TextEffect::scaleSize(begin, end, size));
		}

		// Get text effects.
		Array<TextEffect> *STORM_FN effects() const;

		// Peek at the effects (i.e. don't get a copy of them).
		Array<TextEffect> *peekEffects() const { return myEffects; }

		// Get the backend-specific representation for painting to "graphics".
		void *backendLayout(Graphics *graphics);

	private:
		// The text layout.
		UNKNOWN(PTR_NOGC) void *layout;

		// Destructor for the data.
		typedef void (*Cleanup)(void *);
		UNKNOWN(PTR_NOGC) Cleanup cleanup;

		// Text manager.
		UNKNOWN(PTR_NOGC) TextMgr *mgr;

		// The text we represent.
		Str *myText;

		// Which font are we using?
		Font *myFont;

		// Border of the layout.
		Size myBorder;

		// All text effects applied here. Effects before 'appliedEffects' are known to be
		// non-overlapping for a particular effect type. Effects after are just a copy of
		// the effects to be applied eventually.
		Array<TextEffect> *myEffects;

		// How many of the effects have been applied so far?
		Nat appliedEffects;

		// Common initialization.
		void init();

		// Re-create the layout.
		void recreate();

		// Insert the effect at 'appliedEffects' sorted in the array, and merge it as appropriately.
		void insertEffect(TextEffect effect);

		// Remove an effect from 'myEffect'.
		void removeEffectI(Nat index);

		// Insert an effect into the end of 'myEffect' (i.e. right before 'appliedEffects'. Possibly
		// reusing any empty slots already there.
		void insertEffectI(TextEffect effect);

		// Clean up any empty effects on the end of the array.
		void cleanupEffects();
	};

}
