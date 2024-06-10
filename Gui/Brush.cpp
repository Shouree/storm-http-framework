#include "stdafx.h"
#include "Brush.h"
#include "Painter.h"
#include "Bitmap.h"
#include "GraphicsMgr.h"

namespace gui {

	Brush::Brush() {}

	SolidBrush::SolidBrush(Color c) : myOpacity(1.0f), myColor(c) {}

	void SolidBrush::create(GraphicsMgrRaw *g, void *&result, Cleanup &update) {
		g->create(this, result, update);
	}
	void SolidBrush::update(GraphicsMgrRaw *g, void *resource) {
		g->update(this, resource);
	}


	BitmapBrush::BitmapBrush(Bitmap *bitmap) : myBitmap(bitmap), myTfm(new (engine()) Transform()), myOpacity(1.0f) {}

	BitmapBrush::BitmapBrush(Bitmap *bitmap, Transform *tfm) : myBitmap(bitmap), myTfm(tfm), myOpacity(1.0f) {}

	void BitmapBrush::transform(Transform *tfm) {
		myTfm = tfm;
		needUpdate();
	}

	void BitmapBrush::create(GraphicsMgrRaw *g, void *&result, Cleanup &update) {
		g->create(this, result, update);
	}
	void BitmapBrush::update(GraphicsMgrRaw *g, void *resource) {
		g->update(this, resource);
	}


	GradientStop::GradientStop(Float position, Color color) : pos(position), color(color) {}

	wostream &operator <<(wostream &to, const GradientStop &s) {
		return to << s.color << L"@" << s.pos;
	}

	void GradientStop::toS(StrBuf *to) const {
		*to << color << S("@") << pos;
	}


	Gradient::Gradient() : myStops(null) {}

	Gradient::Gradient(Array<GradientStop> *stops) : myStops(stops) {}

	Array<GradientStop> *Gradient::stops() const {
		return new (this) Array<GradientStop>(*myStops);
	}

	void Gradient::stops(Array<GradientStop> *stops) {
		myStops = new (this) Array<GradientStop>(*stops);

		// We need to re-create the backend-specific things.
		recreate();
	}


	LinearGradient::LinearGradient(Array<GradientStop> *stops, Point start, Point end) :
		Gradient(stops), myStart(start), myEnd(end) {}

	LinearGradient::LinearGradient(Color c1, Color c2, Point start, Point end) :
		Gradient(), myStart(start), myEnd(end) {

		Array<GradientStop> *s = new (this) Array<GradientStop>();
		s->push(GradientStop(0, c1));
		s->push(GradientStop(1, c2));
		stops(s);
	}

	void LinearGradient::start(Point p) {
		myStart = p;
		needUpdate();
	}

	void LinearGradient::end(Point p) {
		myEnd = p;
		needUpdate();
	}

	void LinearGradient::points(Point start, Point end) {
		myStart = start;
		myEnd = end;
		needUpdate();
	}

	void LinearGradient::create(GraphicsMgrRaw *g, void *&result, Cleanup &update) {
		g->create(this, result, update);
	}
	void LinearGradient::update(GraphicsMgrRaw *g, void *resource) {
		g->update(this, resource);
	}


	RadialGradient::RadialGradient(Array<GradientStop> *stops, Point center, Float radius) :
		Gradient(stops), myCenter(center), myRadius(radius), myTransform(new (engine()) Transform()) {}

	RadialGradient::RadialGradient(Color c1, Color c2, Point center, Float radius) :
		Gradient(), myCenter(center), myRadius(radius), myTransform(new (engine()) Transform()) {

		Array<GradientStop> *s = new (this) Array<GradientStop>();
		s->push(GradientStop(0, c1));
		s->push(GradientStop(1, c2));
		stops(s);
	}

	void RadialGradient::center(Point p) {
		myCenter = p;
		needUpdate();
	}

	void RadialGradient::radius(Float v) {
		myRadius = v;
		needUpdate();
	}

	void RadialGradient::transform(Transform *tfm) {
		myTransform = tfm;
		needUpdate();
	}

	void RadialGradient::create(GraphicsMgrRaw *g, void *&result, Cleanup &update) {
		g->create(this, result, update);
	}
	void RadialGradient::update(GraphicsMgrRaw *g, void *resource) {
		g->update(this, resource);
	}

}
