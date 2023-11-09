#pragma once
#include "Core/TObject.h"
#include "Core/Array.h"
#include "Core/WeakSet.h"
#include "Font.h"

namespace gui {
	class Painter;
	class Brush;
	class Text;
	class Path;
	class Bitmap;
	class Resource;
	class GraphicsMgrRaw;
	class SolidBrush;
	class LinearGradient;
	class RadialGradient;

	/**
	 * Generic interface for drawing somewhere.
	 *
	 * Created automatically if used with a painter. See the WindowGraphics class for the
	 * implementation.
	 *
	 * Each painter contains an identifier that is used to associate a graphics instance with
	 * resources used by it. This allows multiple Graphics objects for different backends to be
	 * active simultaneously, and even draw using the same set of graphics resources. Additionally,
	 * different backends have different characteristics regarding how they handle their
	 * resources. Some backends (e.g. software backends) do not require any particular
	 * representation of resources, others (e.g. D2D) can use one internal representation throughout
	 * all Graphics objects, and yet others (e.g. OpenGL-based backends on Gtk) require
	 * instace-specific representations. This is reflected in how identifiers are allocated.
	 */
	class Graphics : public ObjectOn<Render> {
		STORM_ABSTRACT_CLASS;
	public:
		// Create.
		STORM_CTOR Graphics();

		// Make sure we destroy our resources.
		~Graphics();

		// Get the identifier for this Graphics object.
		Nat STORM_FN id() const { return identifier; }

		// Get the graphics manager associated with this object. It is used to determine how
		// resources are created. Graphics objects that do not create backend-specific resources
		// will return a dummy implementation that asserts.
		GraphicsMgrRaw *manager() const { return mgr; }

		// Called when a resource tied to this graphics object is created. Returns 'true' if the
		// resource was newly added here, and false otherwise.
		Bool STORM_FN attach(Resource *resource);

		// Destroy all resources associated to this object.
		virtual void STORM_FN destroy();

		/**
		 * General format. Use push and pop to save/restore the state.
		 */

		// Clear all state on the stack.
		virtual void STORM_FN reset();

		// Push the current state on the state stack.
		virtual void STORM_FN push() ABSTRACT;

		// Push the current state with a modified opacity applied to the new state.
		virtual void STORM_FN push(Float opacity) ABSTRACT;

		// Push the current state with a clipping rectangle applied to the new state.
		virtual void STORM_FN push(Rect clip) ABSTRACT;

		// Push current state, clipping and opacity.
		virtual void STORM_FN push(Rect clip, Float opacity) ABSTRACT;

		// Pop the previous state. Returns false if nothing more to pop.
		virtual Bool STORM_FN pop() ABSTRACT;

		// Set the transform (in relation to the previous state). This is an assignment function, so
		// it can be used as a member variable.
		virtual void STORM_ASSIGN transform(Transform *tfm) ABSTRACT;

		// Set the line width (in relation to the previous state). This is an assignment function,
		// so it can be used as a member variable.
		virtual void STORM_ASSIGN lineWidth(Float w) ABSTRACT;

		/**
		 * Draw stuff.
		 */

		// Draw a line.
		virtual void STORM_FN line(Point from, Point to, Brush *brush) ABSTRACT;

		// Draw a rectangle.
		virtual void STORM_FN draw(Rect rect, Brush *brush) ABSTRACT;

		// Draw rounded rectangle.
		virtual void STORM_FN draw(Rect rect, Size edges, Brush *brush) ABSTRACT;

		// Draw an oval.
		virtual void STORM_FN oval(Rect rect, Brush *brush) ABSTRACT;

		// Draw a path.
		virtual void STORM_FN draw(Path *path, Brush *brush) ABSTRACT;

		// Fill a rectangle.
		virtual void STORM_FN fill(Rect rect, Brush *brush) ABSTRACT;

		// Fill rounded rectangle.
		virtual void STORM_FN fill(Rect rect, Size edges, Brush *brush) ABSTRACT;

		// Fill the entire area.
		virtual void STORM_FN fill(Brush *brush) ABSTRACT;

		// Fill a path.
		virtual void STORM_FN fill(Path *path, Brush *brush) ABSTRACT;

		// Fill an oval.
		virtual void STORM_FN fillOval(Rect rect, Brush *brush) ABSTRACT;

		// Draw an entire bitmap at (0, 0).
		void STORM_FN draw(Bitmap *bitmap);

		// Draw an entire bitmap at `topLeft`.
		void STORM_FN draw(Bitmap *bitmap, Point topLeft);

		// Draw an entire bitmap at `topLeft` with the specified opacity.
		void STORM_FN draw(Bitmap *bitmap, Point topLeft, Float opacity);

		// Draw an entire bitmap. Scale it to fit inside `rect`.
		void STORM_FN draw(Bitmap *bitmap, Rect rect);

		// Draw an entire bitmap. Scale it to fit inside `rect`. Apply the specified opacity.
		virtual void STORM_FN draw(Bitmap *bitmap, Rect rect, Float opacity) ABSTRACT;

		// Draw the part of the bitmap indicated in `src` (in pixels). Draw it at `topLeft`.
		void STORM_FN draw(Bitmap *bitmap, Rect src, Point topLeft);

		// Draw the part of the bitmap indicated in `src` (in pixels). Draw it at
		// `topLeft` with the specified opactiy.
		void STORM_FN draw(Bitmap *bitmap, Rect src, Point topLeft, Float opacity);

		// Draw the part of the bitmap indicated in `src` (in pixels). Scale the resulting pixels to fit inside `dest`.
		void STORM_FN draw(Bitmap *bitmap, Rect src, Rect dest);

		// Draw the part of the bitmap indicated in `src` (in pixels). Scale the resulting pixels to
		// fit inside `dest`, and apply the specified opacity.
		virtual void STORM_FN draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity) ABSTRACT;

		// Draw text the easy way. Prefer using a Text object if possible, since they give more
		// control over the text and yeild better performance. Attempts to fit the given text inside
		// `rect`. No attempt is made at clipping anything not fitting inside `rect`, so any text
		// not fitting inside `rect` will be drawn outside the rectangle.
		virtual void STORM_FN text(Str *text, Font *font, Brush *brush, Rect rect) ABSTRACT;

		// Draw pre-formatted text.
		virtual void STORM_FN draw(Text *text, Brush *brush, Point origin) ABSTRACT;

	protected:
		// Our identifier. Initialized to 0 (meaning, we don't need resources), but set by some
		// subclasses during creation.
		Nat identifier;

		// Replace the graphics manager.
		void STORM_ASSIGN manager(GraphicsMgrRaw *m) { mgr = m; }

	private:
		// Resources associated with this graphics object. I.e. instances that have created
		// something that is tied to this Graphics instance.
		WeakSet<Resource> *resources;

		// Graphics manager. Created during construction.
		GraphicsMgrRaw *mgr;

	};


}
