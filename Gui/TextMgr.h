#pragma once
#include "Font.h"
#include "Text.h"

namespace gui {

	/**
	 * A resource manager for Text objects.
	 *
	 * A Device has to provide an implementation of this object in order to determine how Text is
	 * laid out by the Text class. This allows the backend to create and cache any data that is
	 * needed for fast font rendering for the specific backend.
	 *
	 * This class works much like GraphicsMgr, with the major difference that there may only be one
	 * TextMgr active in the system at any given point.
	 */
	class TextMgr : NoCopy {
	public:
		// Function for destroying resources. Must refer to a static function somewhere, as it is not GC:d.
		// The main reason we have a cleanup function rather than another virtual function here is that the TextMgr
		// might be deallocated before all finalizers have been executed during shutdown, which could lead to crashes.
		typedef void (*CleanupFn)(void *);

		// Generic resource.
		struct Resource {
			// Create an empty resource.
			Resource() : data(null), cleanup(null) {}

			// Create it.
			Resource(void *data, CleanupFn cleanup)
				: data(data), cleanup(cleanup) {}

			// Allocated data of some kind.
			void *data;

			// Function used to clean up the data.
			CleanupFn cleanup;

			// Clean up this resource.
			void clear() {
				if (data && cleanup)
					(*cleanup)(data);
				data = null;
				cleanup = null;
			}
		};

		/**
		 * Font management.
		 *
		 * Each Font object is able to store a backend-specific pointer of font-data. This is used
		 * by backends to cache font resources that are specific to that backend, in order to speed
		 * up text layout.
		 *
		 * Fonts are created lazily by the system in general.
		 *
		 * Font resources are never changed, if the user changes a Font object, the underlying
		 * object is simply re-created whenever it is needed again.
		 */

		// Create a font resource.
		virtual Resource createFont(const Font *font) = 0;


		/**
		 * Text layout management.
		 *
		 * Creates a text layout object that is passed to a GraphicsMgr at a later stage. This
		 * object is expected to contain all data required to render a portion of text.
		 *
		 * The object is created lazily by the system.
		 *
		 * There are a number of functions that intend to modify the laid-out text. These may or may
		 * not be supported by the backend. If a particular function is not supported, it returns
		 * false. That means that the system will re-create the entire text layout instead. All such
		 * functions have default implementations here that simply return false.
		 */

		// Create a text layout resource. Unless we re-create the layout, it is safe to assume that
		// the effects array is empty.
		virtual Resource createLayout(const Text *text) = 0;

		// Update the layout border of a layout.
		virtual bool updateBorder(void *layout, Size border) { return false; }

		// Result that indicate how to handle effects.
		enum EffectResult {
			// Applied now.
			eApplied,

			// Must wait until rendering.
			eWait,

			// Must re-create the entire layout.
			eReCreate
		};

		// Add a new effect to the layout. Note: "graphics" may be null if we are not currently rendering.
		virtual EffectResult addEffect(void *layout, const TextEffect &effect, Font *font, Str *text, MAYBE(Graphics *) graphics) = 0;

		// Get the actual size of the layout.
		virtual Size size(void *layout) = 0;

		// Get information about each line of the formatted text.
		virtual Array<TextLine *> *lineInfo(void *layout, Text *text) = 0;

		// Get a set of rectangles that cover a range of characters.
		virtual Array<Rect> *boundsOf(void *layout, Text *text, Str::Iter begin, Str::Iter end) = 0;
	};

}
