#include "stdafx.h"
#include "Manager.h"
#include "Skia.h"
#include "LocalShader.h"
#include "Device.h"
#include "Exception.h"

namespace gui {

	SkiaManager::SkiaManager(Graphics *owner, SkiaSurface &surface) : owner(owner), surface(&surface) {}

#ifdef GUI_ENABLE_SKIA

	// Everything in here is a "SkPaint"
	static void cleanupPaint(void *param) {
		SkPaint *paint = (SkPaint *)param;
		delete paint;
	}

	void SkiaManager::create(SolidBrush *brush, void *&result, Resource::Cleanup &cleanup) {
		Color c = brush->color();
		c.a *= brush->opacity();
		SkPaint *paint = new SkPaint(skia(c));
		paint->setAntiAlias(true);

		cleanup = &cleanupPaint;
		result = paint;
	}

	void SkiaManager::update(SolidBrush *brush, void *resource) {
		SkPaint *paint = (SkPaint *)resource;

		Color c = brush->color();
		c.a *= brush->opacity();
		paint->setColor(skia(c));
	}

	void SkiaManager::create(BitmapBrush *brush, void *&result, Resource::Cleanup &cleanup) {
		surface->makeCurrent();

		SkPaint *paint = new SkPaint();
		paint->setAntiAlias(true);

		SkiaBitmap *bitmap = (SkiaBitmap *)brush->bitmap()->forGraphicsRaw(owner);

		if (!bitmap->brushShader) {
			SkSamplingOptions sampling(SkFilterMode::kLinear, SkMipmapMode::kNearest);
			bitmap->brushShader = bitmap->image->makeShader(SkTileMode::kRepeat, SkTileMode::kRepeat, sampling);
		}

		sk_sp<LocalShader> shader = sk_make_sp<LocalShader>(bitmap->brushShader, skia(brush->transform()));
		paint->setShader(shader);

		result = paint;
		cleanup = &cleanupPaint;
	}

	void SkiaManager::update(BitmapBrush *brush, void *resource) {
		SkPaint *paint = (SkPaint *)resource;
		SkShader *shader = paint->getShader();
		// We always use a LocalShader in between.
		LocalShader *local = (LocalShader *)shader;
		local->matrix = skia(brush->transform());
	}

	static SkMatrix linearMatrix(Point start, Point end) {
		Point delta = end - start;
		Angle a = angle(delta);
		Float scale = delta.length();

		SkMatrix tfm = SkMatrix::RotateRad(a.rad());
		tfm.postScale(scale, scale);
		tfm.postTranslate(start.x, start.y);
		return tfm;
	}

	void SkiaManager::create(LinearGradient *brush, void *&result, Resource::Cleanup &cleanup) {
		surface->makeCurrent();

		SkPaint *paint = new SkPaint();
		paint->setAntiAlias(true);

		Array<GradientStop> *stops = brush->peekStops();
		// We transform these points later. Using (0, 0) - (0, -1) makes it easy to compute this transform.
		SkPoint points[2] = {
			{ 0, 0 },
			{ 0, -1 },
		};
		SkColor *colors = (SkColor *)alloca(stops->count() * sizeof(SkColor));
		SkScalar *pos = (SkScalar *)alloca(stops->count() * sizeof(SkScalar));
		for (Nat i = 0; i < stops->count(); i++) {
			colors[i] = skia(stops->at(i).color).toSkColor();
			pos[i] = stops->at(i).pos;
		}
		sk_sp<SkShader> shader = SkGradientShader::MakeLinear(points, colors, pos, stops->count(), SkTileMode::kClamp);
		shader = sk_make_sp<LocalShader>(shader, linearMatrix(brush->start(), brush->end()));
		paint->setShader(shader);

		result = paint;
		cleanup = &cleanupPaint;
	}

	void SkiaManager::update(LinearGradient *brush, void *resource) {
		SkPaint *paint = (SkPaint *)resource;
		SkShader *shader = paint->getShader();
		// We always use a LocalShader in between.
		LocalShader *local = (LocalShader *)shader;
		local->matrix = linearMatrix(brush->start(), brush->end());
	}

	static SkMatrix radialMatrix(RadialGradient *brush) {
		Float r = brush->radius();

		SkMatrix tfm = SkMatrix::Scale(r, r);
		tfm.postTranslate(brush->center().x, brush->center().y);
		tfm.postConcat(skia(brush->transform()));
		return tfm;
	}

	void SkiaManager::create(RadialGradient *brush, void *&result, Resource::Cleanup &cleanup) {
		surface->makeCurrent();

		SkPaint *paint = new SkPaint();
		paint->setAntiAlias(true);

		Array<GradientStop> *stops = brush->peekStops();
		SkColor *colors = (SkColor *)alloca(stops->count() * sizeof(SkColor));
		SkScalar *pos = (SkScalar *)alloca(stops->count() * sizeof(SkScalar));
		for (Nat i = 0; i < stops->count(); i++) {
			colors[i] = skia(stops->at(i).color).toSkColor();
			pos[i] = stops->at(i).pos;
		}

		// We create a radial gradient with a center of 0, 0 and a radius of 1.0 and then use the matrix to scale and move it.
		sk_sp<SkShader> shader = SkGradientShader::MakeRadial(SkPoint::Make(0, 0), 1.0, colors, pos, stops->count(), SkTileMode::kClamp);
		shader = sk_make_sp<LocalShader>(shader, radialMatrix(brush));
		paint->setShader(shader);

		result = paint;
		cleanup = &cleanupPaint;
	}

	void SkiaManager::update(RadialGradient *brush, void *resource) {
		SkPaint *paint = (SkPaint *)resource;
		SkShader *shader = paint->getShader();
		// We always use a LocalShader in between.
		LocalShader *local = (LocalShader *)shader;
		local->matrix = radialMatrix(brush);
	}

	static void cleanupBitmap(void *ptr) {
		SkiaBitmap *b = (SkiaBitmap *)ptr;
		delete b;
	}

	void SkiaManager::create(Bitmap *src, void *&result, Resource::Cleanup &cleanup) {
		surface->makeCurrent();

		Image *image = src->image();
		SkImageInfo info = SkImageInfo::Make(image->width(), image->height(), kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
		SkPixmap pixmap(info, image->buffer(), image->stride());

		SkiaBitmap *bitmap = new SkiaBitmap();
		bitmap->image = SkImages::CrossContextTextureFromPixmap(surface->device(), pixmap, false, true);

		result = bitmap;
		cleanup = &cleanupBitmap;
	}

	void SkiaManager::update(Bitmap *, void *) {
		// Should never be called.
	}

	static void cleanupPath(void *ptr) {
		SkPath *p = (SkPath *)ptr;
		delete p;
	}

	void SkiaManager::create(Path *path, void *&result, Resource::Cleanup &cleanup) {
		surface->makeCurrent();

		SkPathBuilder builder(SkPathFillType::kEvenOdd);

		Bool started = false;
		Array<PathPoint> *elements = path->peekData();
		for (Nat i = 0; i < elements->count(); i++) {
			PathPoint &e = elements->at(i);
			switch (e.t) {
			case tStart:
				builder.moveTo(skia(e.start()->pt));
				started = true;
				break;
			case tClose:
				builder.close();
				started = false;
				break;
			case tLine:
				if (started) {
					builder.lineTo(skia(e.line()->to));
				}
				break;
			case tBezier2:
				if (started) {
					builder.quadTo(skia(e.bezier2()->c1), skia(e.bezier2()->to));
				}
				break;
			case tBezier3:
				if (started) {
					builder.cubicTo(skia(e.bezier3()->c1), skia(e.bezier3()->c2), skia(e.bezier3()->to));
				}
				break;
			}
		}

		result = new SkPath(builder.detach());
		cleanup = &cleanupPath;
	}

	void SkiaManager::update(Path *, void *) {
		// Should never be called.
	}

#else

	DEFINE_GRAPHICS_MGR_FNS(SkiaManager);

#endif

}
