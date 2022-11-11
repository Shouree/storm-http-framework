#include "stdafx.h"
#include "Graphics.h"
#include "Device.h"
#include "Manager.h"
#include "Text.h"
#include "TextMgr.h"

namespace gui {

	SkiaGraphics::SkiaGraphics(SkiaSurface &surface, Nat id) : surface(surface) {
		identifier = id;
		rendering = false;

		states = new (this) Array<State>();

#ifdef GUI_ENABLE_SKIA
		states->push(State());
		manager(new (this) SkiaManager(this, surface));
#endif
	}

	SkiaGraphics::~SkiaGraphics() {}

#ifdef GUI_ENABLE_SKIA

	void SkiaGraphics::destroy() {
		// Make sure the GL context is active before destroying things.
		surface.makeCurrent();
		WindowGraphics::destroy();
	}

	void SkiaGraphics::surfaceResized() {
		// Not needed.
	}

	void SkiaGraphics::surfaceDestroyed() {
		// Not needed.
	}

	/**
	 * State management.
	 */

	void SkiaGraphics::beforeRender(Color bgColor) {
		surface.makeCurrent();

		rendering = true;

		State state(SkMatrix::Scale(surface.scale, surface.scale), 1.0f);
		surface.canvas->setMatrix(state.matrix());
		states->last() = state;

		SkPaint paint(skia(bgColor));
		paint.setStroke(false);
		surface.canvas->drawPaint(paint);
	}

	bool SkiaGraphics::afterRender() {
		surface.makeCurrent();

		// Make sure all layers are returned to the stack.
		reset();

		// Flush operations so that it is safe to present the contents of the texture.
		surface.surface->flushAndSubmit();

		rendering = false;

		return true;
	}


	/**
	 * State management.
	 */

	void SkiaGraphics::reset() {
		if (!rendering)
			return;

		// Clear any remaining states from the stack.
		while (pop())
			;
	}

	void SkiaGraphics::pushState() {
		if (!rendering)
			return;

		surface.makeCurrent();

		states->push(State(surface.canvas->getTotalMatrix(), lineW));
	}

	void SkiaGraphics::push() {
		if (!rendering)
			return;

		surface.makeCurrent();

		surface.canvas->save();
		pushState();
	}

	void SkiaGraphics::push(Float opacity) {
		if (!rendering)
			return;

		surface.makeCurrent();

		if (opacity >= 1.0f) {
			surface.canvas->save();
		} else {
			// TODO: Hints for the layer size according to clip? Skia probably does that anyway...
			SkPaint paint;
			paint.setAlphaf(opacity);
			paint.setAntiAlias(true);
			surface.canvas->saveLayer(nullptr, &paint);
		}
		pushState();
	}

	void SkiaGraphics::push(Rect clip) {
		if (!rendering)
			return;

		surface.makeCurrent();

		surface.canvas->save();
		pushState();

		surface.canvas->clipRect(skia(clip), SkClipOp::kIntersect, true);
	}

	void SkiaGraphics::push(Rect clip, Float opacity) {
		if (!rendering)
			return;

		surface.makeCurrent();

		// TODO: Is the layer size suggestion in device units (I guess so) or in transformed units?
		SkRect rect = skia(clip);
		SkPaint paint;
		paint.setAlphaf(opacity);
		paint.setAntiAlias(true);
		surface.canvas->saveLayer(&rect, &paint);
		pushState();

		surface.canvas->clipRect(skia(clip), SkClipOp::kIntersect, true);
	}

	Bool SkiaGraphics::pop() {
		if (!rendering)
			return false;

		surface.makeCurrent();

		if (states->count() <= 1)
			return false;

		states->pop();
		surface.canvas->restore();

		return true;
	}

	void SkiaGraphics::transform(Transform *tfm) {
		if (!rendering)
			return;

		SkMatrix m = states->last().matrix();
		m.preConcat(skia(tfm));
		surface.canvas->setMatrix(m);
	}

	void SkiaGraphics::lineWidth(Float w) {
		lineW = states->last().lineWidth * w;
	}


	/**
	 * Draw stuff.
	 */

	SkPaint *SkiaGraphics::paint(Brush *style, Bool stroke) {
		if (!rendering)
			return;

		SkPaint *paint = (SkPaint *)style->forGraphicsRaw(this);
		paint->setStroke(stroke);
		paint->setStrokeWidth(lineW);
		return paint;
	}

	void SkiaGraphics::line(Point from, Point to, Brush *style) {
		if (!rendering)
			return;

		surface.makeCurrent();
		surface.canvas->drawLine(skia(from), skia(to), *paint(style, true));
	}

	void SkiaGraphics::draw(Rect rect, Brush *style) {
		if (!rendering)
			return;

		surface.makeCurrent();
		surface.canvas->drawRect(skia(rect), *paint(style, true));
	}

	void SkiaGraphics::draw(Rect rect, Size edges, Brush *style) {
		if (!rendering)
			return;

		surface.makeCurrent();
		surface.canvas->drawRRect(SkRRect::MakeRectXY(skia(rect), edges.w, edges.h), *paint(style, true));
	}

	void SkiaGraphics::oval(Rect rect, Brush *style) {
		if (!rendering)
			return;

		surface.makeCurrent();
		surface.canvas->drawOval(skia(rect), *paint(style, true));
	}

	void SkiaGraphics::draw(Path *path, Brush *style) {
		if (!rendering)
			return;

		surface.makeCurrent();
		SkPath *p = (SkPath *)path->forGraphicsRaw(this);
		surface.canvas->drawPath(*p, *paint(style, true));
	}

	void SkiaGraphics::fill(Rect rect, Brush *style) {
		if (!rendering)
			return;

		surface.makeCurrent();
		surface.canvas->drawRect(skia(rect), *paint(style, false));
	}

	void SkiaGraphics::fill(Rect rect, Size edges, Brush *style) {
		if (!rendering)
			return;

		surface.makeCurrent();
		surface.canvas->drawRRect(SkRRect::MakeRectXY(skia(rect), edges.w, edges.h), *paint(style, false));
	}

	void SkiaGraphics::fill(Brush *style) {
		if (!rendering)
			return;

		surface.makeCurrent();
		surface.canvas->drawPaint(*paint(style, false));
	}

	void SkiaGraphics::fillOval(Rect rect, Brush *style) {
		if (!rendering)
			return;

		surface.makeCurrent();
		surface.canvas->drawOval(skia(rect), *paint(style, false));
	}

	void SkiaGraphics::fill(Path *path, Brush *style) {
		if (!rendering)
			return;

		surface.makeCurrent();
		SkPath *p = (SkPath *)path->forGraphicsRaw(this);
		surface.canvas->drawPath(*p, *paint(style, false));
	}

	void SkiaGraphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		if (!rendering)
			return;

		surface.makeCurrent();
		SkPaint paint;
		paint.setAntiAlias(true);
		paint.setAlphaf(opacity);
		paint.setFilterQuality(kMedium_SkFilterQuality); // For linear interpolation w/ mipmaps if available

		SkiaBitmap *b = (SkiaBitmap *)bitmap->forGraphicsRaw(this);
		surface.canvas->drawImageRect(b->image, skia(rect), &paint);
	}

	void SkiaGraphics::draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity) {
		if (!rendering)
			return;

		surface.makeCurrent();
		SkPaint paint;
		paint.setAntiAlias(true);
		paint.setAlphaf(opacity);
		paint.setFilterQuality(kMedium_SkFilterQuality); // For linear interpolation w/ mipmaps if available

		SkiaBitmap *b = (SkiaBitmap *)bitmap->forGraphicsRaw(this);
		surface.canvas->drawImageRect(b->image, skia(src), skia(dest), &paint);
	}

	void SkiaGraphics::text(Str *text, Font *font, Brush *style, Rect rect) {
		if (!rendering)
			return;

		surface.makeCurrent();
		// We're creating some extra pressure on the GC here, but we don't want to re-implement all
		// the logic in the backend here.
		Text *t = new (this) Text(text, font, rect.size());
		draw(t, style, rect.p0);
	}

	void SkiaGraphics::draw(Text *text, Brush *style, Point origin) {
		if (!rendering)
			return;

		surface.makeCurrent();
		SkiaText *p = (SkiaText *)text->backendLayout(this);
		p->draw(*surface.canvas, *paint(style, false), origin);
	}

#else

	void SkiaGraphics::destroy() {}

	DEFINE_WINDOW_GRAPHICS_FNS(SkiaGraphics)

#endif

}
