#pragma once
#include "Resource.h"
#include "Core/Array.h"

namespace gui {
	class Bitmap;

	/**
	 * Brush (abstract).
	 */
	class Brush : public Resource {
		STORM_CLASS;
	public:
		// Create a brush.
		Brush();
	};

	/**
	 * Solid brush.
	 */
	class SolidBrush : public Brush {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR SolidBrush(Color color);

		// Opacity.
		inline Float STORM_FN opacity() const { return myOpacity; }
		inline void STORM_ASSIGN opacity(Float o) { myOpacity = o; needUpdate(); }

		// Color.
		inline Color STORM_FN color() const { return myColor; }
		inline void STORM_ASSIGN color(Color c) { myColor = c; needUpdate(); }

	protected:
		// Create and update.
		void create(GraphicsMgrRaw *g, void *&result, Cleanup &clean);
		void update(GraphicsMgrRaw *g, void *resource);

	private:
		// Color.
		Color myColor;

		// Opacity.
		Float myOpacity;
	};


	/**
	 * Bitmap brush.
	 */
	class BitmapBrush : public Brush {
		STORM_CLASS;
	public:
		STORM_CTOR BitmapBrush(Bitmap *bitmap);
		STORM_CTOR BitmapBrush(Bitmap *bitmap, Transform *tfm);

		// Get the bitmap.
		inline Bitmap *STORM_FN bitmap() const { return myBitmap; }

		// Get the current transform.
		inline Transform *STORM_FN transform() const { return myTfm; }

		// Set the current transform.
		void STORM_ASSIGN transform(Transform *tfm);

		// Opacity.
		inline Float STORM_FN opacity() const { return myOpacity; }
		inline void STORM_ASSIGN opacity(Float o) { myOpacity = o; needUpdate(); }

	protected:
		// Create and update.
		void create(GraphicsMgrRaw *g, void *&result, Cleanup &clean);
		void update(GraphicsMgrRaw *g, void *resource);

	private:
		// The actual bitmap.
		Bitmap *myBitmap;

		// The transform when applying the brush.
		Transform *myTfm;

		// Opacity.
		Float myOpacity;
	};


	/**
	 * A single gradient stop.
	 */
	class GradientStop {
		STORM_VALUE;
	public:
		STORM_CTOR GradientStop(Float position, Color color);

		// Position.
		Float pos;

		// Color.
		Color color;

		// Output.
		void STORM_FN toS(StrBuf *to) const;
	};

	// C++ version.
	wostream &operator <<(wostream &to, const GradientStop &s);


	/**
	 * Gradient of some kind.
	 */
	class Gradient : public Brush {
		STORM_CLASS;
	public:
		Gradient();
		Gradient(Array<GradientStop> *stops);

		// Get the stops.
		Array<GradientStop> *STORM_FN stops() const;

		// Set the stops.
		void STORM_ASSIGN stops(Array<GradientStop> *stops);

		// Peek at the stops (doesn't copy it).
		Array<GradientStop> *peekStops() const { return myStops; }

	private:
		// Stops.
		Array<GradientStop> *myStops;
	};


	/**
	 * Linear gradient.
	 */
	class LinearGradient : public Gradient {
		STORM_CLASS;
	public:
		// Provide the stops.
		STORM_CTOR LinearGradient(Array<GradientStop> *stops, Point start, Point end);

		// Create two stops at 0 and 1.
		STORM_CTOR LinearGradient(Color c1, Color c2, Point start, Point end);

		// Get start and end points.
		inline Point STORM_FN start() const { return myStart; }
		inline Point STORM_FN end() const { return myEnd; }

		// Set start and end points.
		void STORM_ASSIGN start(Point p);
		void STORM_ASSIGN end(Point p);

		// Set both.
		void STORM_FN points(Point start, Point end);

	protected:
		// Create and update.
		void create(GraphicsMgrRaw *g, void *&result, Cleanup &clean);
		void update(GraphicsMgrRaw *g, void *resource);

	private:
		// Start and end point of the gradient.
		Point myStart;
		Point myEnd;

#ifdef GUI_GTK
		// Set the transform of 'p'.
		void updatePoints(cairo_pattern_t *p);
#endif
	};


	/**
	 * Radial gradient.
	 */
	class RadialGradient : public Gradient {
		STORM_CLASS;
	public:
		// Provide the stops.
		STORM_CTOR RadialGradient(Array<GradientStop> *stops, Point center, Float radius);

		// Create two stops at 0 and 1.
		STORM_CTOR RadialGradient(Color c1, Color c2, Point center, Float radius);

		// Get/set the center point.
		inline Point STORM_FN center() const { return myCenter; }
		void STORM_ASSIGN center(Point pt);

		// Get/set the radius.
		inline Float STORM_FN radius() const { return myRadius; }
		void STORM_ASSIGN radius(Float radius);

		// Get/set a generic transform.
		Transform *STORM_FN transform() const { return myTransform; }
		void STORM_ASSIGN transform(Transform *tfm);

	protected:
		// Create and update.
		void create(GraphicsMgrRaw *g, void *&result, Cleanup &clean);
		void update(GraphicsMgrRaw *g, void *resource);

	private:
		// Center point and radius.
		Point myCenter;
		Float myRadius;

		// Transform.
		Transform *myTransform;

#ifdef GUI_GTK
		// Set the transform of 'p'.
		void update(cairo_pattern_t *p);
#endif
	};

}
