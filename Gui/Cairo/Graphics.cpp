#include "stdafx.h"
#include "Graphics.h"
#include "Painter.h"
#include "Brush.h"
#include "Text.h"
#include "Path.h"
#include "Bitmap.h"
#include "Device.h"
#include "Manager.h"

namespace gui {

	CairoGraphics::CairoGraphics(CairoSurface &surface, Nat id, Bool flipY) : surface(surface), flipY(flipY) {
		identifier = id;
		rendering = false;

#ifdef GUI_GTK
		manager(new (this) CairoManager(this, &surface));
#endif

		oldStates = new (this) Array<State>();


		state = State();
		oldStates->push(state);
	}

	CairoGraphics::~CairoGraphics() {}

#ifdef GUI_GTK

	void CairoGraphics::surfaceResized() {
		// Not needed.
	}

	void CairoGraphics::surfaceDestroyed() {
		// Not needed.
	}

	/**
	 * State management.
	 */

	void CairoGraphics::beforeRender(Color bgColor) {
		rendering = true;

		// Make sure the cairo context is in a reasonable state.
		cairo_surface_mark_dirty(surface.surface);

		// Clear with the BG color.
		cairo_set_source_rgba(surface.device, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		cairo_set_operator(surface.device, CAIRO_OPERATOR_SOURCE);
		cairo_paint(surface.device);

		// Set defaults, just in case.
		cairo_set_operator(surface.device, CAIRO_OPERATOR_OVER);
		cairo_set_fill_rule(surface.device, CAIRO_FILL_RULE_EVEN_ODD);

		// Update the clip region and scale of the root state.
		state = State();
		state.scale(surface.scale);
		state.clip = Rect(Point(), surface.size);

		if (flipY)
			state.flipY(surface.size.h);

		oldStates->last() = state;

		// Set up the backend.
		prepare();
	}

	bool CairoGraphics::afterRender() {
		// Make sure all layers are returned to the stack.
		reset();

		cairo_surface_flush(surface.surface);
		rendering = false;

		return true;
	}


	/**
	 * State management.
	 */

	void CairoGraphics::reset() {
		// Clear any remaining states from the stack.
		while (pop())
			;
	}

	void CairoGraphics::push() {
		if (!rendering)
			return;

		oldStates->push(state);
		state.type = LayerKind::none;
	}

	void CairoGraphics::push(Float opacity) {
		if (!rendering)
			return;

		oldStates->push(state);
		state.type = LayerKind::group;
		state.opacity = opacity;
		cairo_push_group(surface.device);
	}

	void CairoGraphics::push(Rect clip) {
		if (!rendering)
			return;

		oldStates->push(state);
		state.type = LayerKind::save;
		cairo_save(surface.device);

		Size sz = clip.size();
		cairo_rectangle(surface.device, clip.p0.x, clip.p0.y, sz.w, sz.h);
		cairo_clip(surface.device);
	}

	void CairoGraphics::push(Rect clip, Float opacity) {
		if (!rendering)
			return;

		oldStates->push(state);
		state.type = LayerKind::group;
		state.opacity = opacity;
		cairo_push_group(surface.device);

		Size sz = clip.size();
		cairo_rectangle(surface.device, clip.p0.x, clip.p0.y, sz.w, sz.h);
		cairo_clip(surface.device);
	}

	Bool CairoGraphics::pop() {
		if (!rendering)
			return false;

		switch (state.type) {
		case LayerKind::none:
			break;
		case LayerKind::group:
			cairo_pop_group_to_source(surface.device);
			cairo_paint_with_alpha(surface.device, state.opacity);
			break;
		case LayerKind::save:
			cairo_restore(surface.device);
			break;
		}

		state = oldStates->last();
		prepare();

		if (oldStates->count() > 1) {
			oldStates->pop();
			return true;
		} else {
			return false;
		}
	}

	void CairoGraphics::transform(Transform *tfm) {
		if (!rendering)
			return;

		cairo_matrix_t m = cairoMultiply(cairo(tfm), oldStates->last().transform());
		state.transform(m);
		cairo_set_matrix(surface.device, &m);
	}

	void CairoGraphics::lineWidth(Float w) {
		if (!rendering)
			return;

		// TODO: How is the width of lines affected by scaling etc.? How do we want it to behave?
		state.lineWidth = oldStates->last().lineWidth *w;
		cairo_set_line_width(surface.device, state.lineWidth);
	}

	void CairoGraphics::prepare() {
		if (!rendering)
			return;

		cairo_matrix_t tfm = state.transform();
		cairo_set_matrix(surface.device, &tfm);
		cairo_set_line_width(surface.device, state.lineWidth);
	}


	/**
	 * Draw stuff.
	 */

#define SET_BRUSH(style) CairoManager::applyBrush(surface.device, (style), (style)->forGraphicsRaw(this))
#define GET_BITMAP(bitmap) ((cairo_surface_t *)(bitmap)->forGraphicsRaw(this))

	void CairoGraphics::line(Point from, Point to, Brush *style) {
		if (!rendering)
			return;

		cairo_new_path(surface.device);
		cairo_move_to(surface.device, from.x, from.y);
		cairo_line_to(surface.device, to.x, to.y);

		SET_BRUSH(style);

		cairo_stroke(surface.device);
	}

	void CairoGraphics::draw(Rect rect, Brush *style) {
		if (!rendering)
			return;

		Size sz = rect.size();
		cairo_rectangle(surface.device, rect.p0.x, rect.p0.y, sz.w, sz.h);
		SET_BRUSH(style);
		cairo_stroke(surface.device);
	}

	static void rectangle(cairo_t *device, const Rect &rect) {
		Size sz = rect.size();
		cairo_rectangle(device, rect.p0.x, rect.p0.y, sz.w, sz.h);
	}

	static void rounded_corner(cairo_t *device, Point center, Size scale, double from, double to) {
		cairo_save(device);
		cairo_translate(device, center.x, center.y);
		cairo_scale(device, scale.w, scale.h);
		cairo_arc(device, 0, 0, 1, from, to);
		cairo_restore(device);
	}

	static void rounded_rect(cairo_t *device, Rect rect, Size edges) {
		const double quarter = M_PI / 2;

		cairo_new_path(device);
		rounded_corner(device, Point(rect.p1.x - edges.w, rect.p0.y + edges.h), edges, -quarter, 0);
		rounded_corner(device, Point(rect.p1.x - edges.w, rect.p1.y - edges.h), edges, 0, quarter);
		rounded_corner(device, Point(rect.p0.x + edges.w, rect.p1.y - edges.h), edges, quarter, 2*quarter);
		rounded_corner(device, Point(rect.p0.x + edges.w, rect.p0.y + edges.h), edges, 2*quarter, 3*quarter);
		cairo_close_path(device);
	}

	void CairoGraphics::draw(Rect rect, Size edges, Brush *style) {
		if (!rendering)
			return;

		rounded_rect(surface.device, rect, edges);

		SET_BRUSH(style);
		cairo_stroke(surface.device);
	}

	static void cairo_oval(cairo_t *device, Rect rect) {
		cairo_save(device);

		Point center = rect.center();
		cairo_translate(device, center.x, center.y);
		Size size = rect.size();
		cairo_scale(device, size.w / 2, size.h / 2);

		cairo_new_path(device);
		cairo_arc(device, 0, 0, 1, 0, 2*M_PI);

		cairo_restore(device);
	}

	void CairoGraphics::oval(Rect rect, Brush *style) {
		if (!rendering)
			return;

		cairo_oval(surface.device, rect);

		SET_BRUSH(style);
		cairo_stroke(surface.device);
	}

	static void draw_path(cairo_t *to, Path *path) {
		cairo_new_path(to);

		Array<PathPoint> *data = path->peekData();
		bool started = false;
		Point current;
		for (Nat i = 0; i < data->count(); i++) {
			PathPoint &e = data->at(i);
			switch (e.t) {
			case tStart:
				current = e.start()->pt;
				cairo_move_to(to, current.x, current.y);
				started = true;
				break;
			case tClose:
				cairo_close_path(to);
				started = false;
				break;
			case tLine:
				if (started) {
					current = e.line()->to;
					cairo_line_to(to, current.x, current.y);
				}
				break;
			case tBezier2:
				if (started) {
					Bezier2 *b = e.bezier2();
					Point c1 = current + (2.0f/3.0f)*(b->c1 - current);
					Point c2 = b->to + (2.0f/3.0f)*(b->c1 - b->to);
					current = b->to;
					cairo_curve_to(to, c1.x, c1.y, c2.x, c2.y, current.x, current.y);
				}
				break;
			case tBezier3:
				if (started) {
					Bezier3 *b = e.bezier3();
					current = b->to;
					cairo_curve_to(to, b->c1.x, b->c1.y, b->c2.x, b->c2.y, current.x, current.y);
				}
				break;
			}
		}
	}

	void CairoGraphics::draw(Path *path, Brush *style) {
		if (!rendering)
			return;

		draw_path(surface.device, path);
		SET_BRUSH(style);
		cairo_stroke(surface.device);
	}

	void CairoGraphics::fill(Rect rect, Brush *style) {
		if (!rendering)
			return;

		rectangle(surface.device, rect);
		SET_BRUSH(style);
		cairo_fill(surface.device);
	}

	void CairoGraphics::fill(Rect rect, Size edges, Brush *style) {
		if (!rendering)
			return;

		rounded_rect(surface.device, rect, edges);

		SET_BRUSH(style);
		cairo_fill(surface.device);
	}

	void CairoGraphics::fill(Brush *style) {
		if (!rendering)
			return;

		cairo_matrix_t tfm;
		cairo_matrix_init_identity(&tfm);
		cairo_set_matrix(surface.device, &tfm);

		SET_BRUSH(style);
		cairo_paint(surface.device);

		tfm = state.transform();
		cairo_set_matrix(surface.device, &tfm);
	}

	void CairoGraphics::fillOval(Rect rect, Brush *style) {
		if (!rendering)
			return;

		cairo_oval(surface.device, rect);

		SET_BRUSH(style);
		cairo_fill(surface.device);
	}

	void CairoGraphics::fill(Path *path, Brush *style) {
		if (!rendering)
			return;

		draw_path(surface.device, path);
		SET_BRUSH(style);
		cairo_fill(surface.device);
	}

	void CairoGraphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		if (!rendering)
			return;

		cairo_pattern_t *pattern = cairo_pattern_create_for_surface(GET_BITMAP(bitmap));
		cairo_matrix_t tfm;
		Size original = bitmap->size();
		Size target = rect.size();
		cairo_matrix_init_scale(&tfm, original.w / target.w, original.h / target.h);
		cairo_matrix_translate(&tfm, -rect.p0.x, -rect.p0.y);
		cairo_pattern_set_matrix(pattern, &tfm);
		cairo_set_source(surface.device, pattern);
		if (opacity < 1.0f)
			cairo_paint_with_alpha(surface.device, opacity);
		else
			cairo_paint(surface.device);
		cairo_pattern_destroy(pattern);
	}

	void CairoGraphics::draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity) {
		if (!rendering)
			return;

		cairo_save(surface.device);

		rectangle(surface.device, dest);
		cairo_clip(surface.device);

		cairo_pattern_t *pattern = cairo_pattern_create_for_surface(GET_BITMAP(bitmap));
		cairo_matrix_t tfm;
		Size original = src.size();
		Size target = dest.size();
		cairo_matrix_init_translate(&tfm, src.p0.x, src.p0.y);
		cairo_matrix_scale(&tfm, original.w / target.w, original.h / target.h);
		cairo_matrix_translate(&tfm, -dest.p0.x, -dest.p0.y);
		cairo_pattern_set_matrix(pattern, &tfm);
		cairo_set_source(surface.device, pattern);
		if (opacity < 1.0f)
			cairo_paint_with_alpha(surface.device, opacity);
		else
			cairo_paint(surface.device);
		cairo_pattern_destroy(pattern);

		cairo_restore(surface.device);
	}

	void CairoGraphics::text(Str *text, Font *font, Brush *style, Rect rect) {
		if (!rendering)
			return;

		// We're creating some extra pressure on the GC here, but we don't want to re-implement all
		// the logic in the backend here.
		Text *t = new (this) Text(text, font, rect.size());
		draw(t, style, rect.p0);
	}

	void CairoGraphics::draw(Text *text, Brush *style, Point origin) {
		if (!rendering)
			return;

		SET_BRUSH(style);

		cairo_move_to(surface.device, origin.x, origin.y);
		pango_cairo_show_layout(surface.device, (PangoLayout *)text->backendLayout(this));
	}

#else

	DEFINE_WINDOW_GRAPHICS_FNS(CairoGraphics)

#endif


}
