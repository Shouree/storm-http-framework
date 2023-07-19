#include "stdafx.h"
#include "Window.h"
#include "Container.h"
#include "Frame.h"
#include "App.h"
#include "Painter.h"
#include "Win32Dpi.h"
#include "GtkSignal.h"
#include "GtkEmpty.h"
#include "Env.h"
#include "Exception.h"

// We're using gtk_widget_override_font, as there is no better way of setting custom fonts on widgets...
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

namespace gui {

	// Null is a failure code from eg. GetParent function, should be OK to use as invalid.
	const Handle Window::invalid;

	Window::Window() :
		myHandle(invalid), myParent(null), myRoot(null),
		myVisible(true), myEnabled(true), drawing(false), mouseInside(false),
		gdkWindow(null), gTimer(null),
		myPos(0, 0, 100, 100) /* large enough to not generate warnings in Gtk+ */ {

		App *a = app(engine());
		if (a->headless())
			throw new (this) GuiError(S("Can not create windows when running in headless mode."));

		myText = new (this) Str(L"");
		myFont = a->defaultFont;
	}

	Window::~Window() {
		if (myHandle != invalid) {
			App *a = app(engine());
			a->removeWindow(this);
		}
	}

	ContainerBase *Window::parent() {
		return myParent;
	}

	Frame *Window::rootFrame() {
		return myRoot;
	}

	Handle Window::handle() const {
		return myHandle;
	}

	Size Window::minSize() {
		return Size();
	}

	void Window::attachParent(ContainerBase *parent) {
		myParent = parent;
		if (parent == this)
			myRoot = as<Frame>(this);
		else
			myRoot = parent->myRoot;
	}

	void Window::detachParent() {
		handle(invalid);
		myRoot = null;
	}

	void Window::windowDestroyed() {}

	void Window::parentCreated(nat id) {
		assert(myParent);

		myRoot = myParent->myRoot;

		if (!created())
			if (!create(myParent, id))
				WARNING(L"Failed to create a window...");
	}

	void Window::handle(Handle handle) {
		App *a = app(engine());
		if (myHandle != invalid) {
			if (myPainter)
				myPainter->uiDetach();

			if (myParent == null) {
				// We're a frame (or orphaned).
				destroyWindow(myHandle);
			} else if (!myParent->created()) {
				// Parent has already been destroyed. This means we're destroyed,
				// by our parent, even though we have a handle.

				// If we have a timer, we need to destroy that though.
				clearTimer();
			} else {
				destroyWindow(myHandle);
			}

			// Notify we've been destroyed.
			myHandle = invalid;
			windowDestroyed();

			// DestroyWindow sends WM_CLOSE, which may be handled by this class.
			a->removeWindow(this);
		}

		myHandle = handle;
		if (myHandle != invalid) {
			a->addWindow(this);
			if (myPainter)
				myPainter->uiAttach(this);
		}
	}

	Bool Window::onKey(Bool down, key::Key id, Modifiers modifiers) {
		return false;
	}

	Bool Window::onChar(Nat id) {
		return false;
	}

	Bool Window::onClick(Bool pressed, Point at, mouse::MouseButton button) {
		return false;
	}

	Bool Window::onDblClick(Point at, mouse::MouseButton button) {
		return false;
	}

	Bool Window::onMouseMove(Point at) {
		return false;
	}

	void Window::onMouseEnter() {}

	void Window::onMouseLeave() {}

	Bool Window::onMouseVScroll(Point at, Int delta) {
		return false;
	}

	Bool Window::onMouseHScroll(Point at, Int delta) {
		return false;
	}

	Bool Window::visible() {
		return myVisible;
	}

	Bool Window::enabled() {
		return myEnabled;
	}

	Font *Window::font() {
		return new (this) Font(*myFont);
	}

	void Window::resized(Size size) {
		// Nothing here, override when needed.
	}

	void Window::resized() {
		resized(pos().size());
	}

	void Window::onResize(Size size) {
		resized(size);
	}

	void Window::painter(MAYBE(Painter *) p) {
		if (myPainter)
			myPainter->uiDetach();

		Painter *old = myPainter;
		myPainter = p;

		if (old) {
			if (!p)
				detachPainter();
		} else {
			if (p)
				attachPainter();
		}

		if (created() && myPainter)
			myPainter->uiAttach(this);
	}

	MAYBE(Painter *) Window::painter() {
		return myPainter;
	}

	void Window::onTimer() {}

#ifdef GUI_WIN32

	void Window::destroyWindow(Handle handle) {
		DestroyWindow(handle.hwnd());
	}

	MsgResult Window::onMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_SIZE: {
			Size s = pos().size();
			if (myPainter) {
				RECT r;
				GetClientRect(handle().hwnd(), &r);
				myPainter->uiResize(Size(Float(r.right), Float(r.bottom)), dpiScale(currentDpi()));
			}
			onResize(s);
			return msgResult(0);
		}
		case WM_PAINT:
			return onPaint();
		case WM_TIMER: {
			if (msg.wParam == 1) {
				onTimer();
				return msgResult(0);
			}
		}
			// TODO: Propagate mouse messages to parent windows?
		case WM_LBUTTONDOWN:
			if (onClick(true, dpiFromPx(currentDpi(), mousePos(msg)), mouse::left))
				return msgResult(0);
			break;
		case WM_LBUTTONUP:
			if (onClick(false, dpiFromPx(currentDpi(), mousePos(msg)), mouse::left))
				return msgResult(0);
			break;
		case WM_LBUTTONDBLCLK:
			onClick(true, dpiFromPx(currentDpi(), mousePos(msg)), mouse::left);
			if (onDblClick(dpiFromPx(currentDpi(), mousePos(msg)), mouse::left))
				return msgResult(0);
			break;
		case WM_MBUTTONDOWN:
			if (onClick(true, dpiFromPx(currentDpi(), mousePos(msg)), mouse::middle))
				return msgResult(0);
			break;
		case WM_MBUTTONUP:
			if (onClick(false, dpiFromPx(currentDpi(), mousePos(msg)), mouse::middle))
				return msgResult(0);
			break;
		case WM_MBUTTONDBLCLK:
			onClick(true, dpiFromPx(currentDpi(), mousePos(msg)), mouse::middle);
			if (onDblClick(dpiFromPx(currentDpi(), mousePos(msg)), mouse::middle))
				return msgResult(0);
			break;
		case WM_RBUTTONDOWN:
			if (onClick(true, dpiFromPx(currentDpi(), mousePos(msg)), mouse::right))
				return msgResult(0);
			break;
		case WM_RBUTTONUP:
			if (onClick(false, dpiFromPx(currentDpi(), mousePos(msg)), mouse::right))
				return msgResult(0);
			break;
		case WM_RBUTTONDBLCLK:
			onClick(true, dpiFromPx(currentDpi(), mousePos(msg)), mouse::right);
			if (onDblClick(dpiFromPx(currentDpi(), mousePos(msg)), mouse::right))
				return msgResult(0);
			break;
		case WM_MOUSEMOVE:
			if (!mouseInside) {
				TRACKMOUSEEVENT track;
				track.cbSize = sizeof(track);
				track.dwFlags = TME_LEAVE;
				track.hwndTrack = handle().hwnd();
				track.dwHoverTime = 0;
				TrackMouseEvent(&track);

				mouseInside = true;
				onMouseEnter();
			}

			if (onMouseMove(dpiFromPx(currentDpi(), mousePos(msg))))
				return msgResult(0);
			break;
		case WM_MOUSEWHEEL:
			if (onMouseVScroll(dpiFromPx(currentDpi(), mouseAbsPos(handle(), msg)), GET_WHEEL_DELTA_WPARAM(msg.wParam)))
				return msgResult(0);
			break;
		case WM_MOUSEHWHEEL:
			if (onMouseHScroll(dpiFromPx(currentDpi(), mouseAbsPos(handle(), msg)), GET_WHEEL_DELTA_WPARAM(msg.wParam)))
				return msgResult(0);
			break;
		case WM_MOUSELEAVE:
			mouseInside = false;
			onMouseLeave();
			break;
		}
		return noResult();
	}

	MsgResult Window::beforeMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (onKey(false, keycode(msg.wParam), modifiers()))
				return msgResult(0);
			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (onKey(true, keycode(msg.wParam), modifiers()))
				return msgResult(0);
			break;
		case WM_CHAR:
			if (onChar(Nat(msg.wParam)))
				return msgResult(0);
			break;
		}
		return noResult();
	}

	void Window::visible(Bool show) {
		myVisible = show;
		if (created())
			ShowWindow(handle().hwnd(), show ? TRUE : FALSE);
	}

	void Window::enabled(Bool enable) {
		myEnabled = enable;
		if (created())
			EnableWindow(handle().hwnd(), enable ? TRUE : FALSE);
	}

	Str *Window::text() {
		if (created()) {
			Nat len = GetWindowTextLength(handle().hwnd());
			GcArray<wchar> *src = runtime::allocArray<wchar>(engine(), &wcharArrayType, len + 1);
			GetWindowText(handle().hwnd(), src->v, len + 1);
			myText = (new (this) Str(src))->fromCrLf();
		}
		return myText;
	}

	void Window::text(Str *s) {
		myText = s;
		if (created())
			SetWindowText(handle().hwnd(), s->toCrLf()->c_str());
	}

	Rect Window::pos() {
		if (created()) {
			// Note: We don't take client borders into account for anything but top-level frames.
			RECT r;
			HWND h = handle().hwnd();
			GetWindowRect(h, &r);

			POINT a = { r.left, r.top };
			POINT b = { r.right, r. bottom };

			if (myParent != this) {
				ScreenToClient(myParent->handle().hwnd(), &a);
				ScreenToClient(myParent->handle().hwnd(), &b);
			}

			myPos = dpiFromPx(currentDpi(), convert(a, b));

			// The Edit control has an extra border (painted outside the control). Take that into account.
			DWORD exStyle = GetWindowLong(h, GWL_EXSTYLE);
			if (exStyle & WS_EX_CLIENTEDGE) {
				myPos = myPos.grow(Size(1));
			}
		}
		return myPos;
	}

	void Window::pos(Rect r) {
		myPos = r;
		if (created()) {
			HWND h = handle().hwnd();

			// The Edit control has an extra border (painted outside the control). Take that into account.
			DWORD exStyle = GetWindowLong(h, GWL_EXSTYLE);
			if (exStyle & WS_EX_CLIENTEDGE) {
				r = r.shrink(Size(1));
			}

			// Note: We don't take client borders into account for anything but top-level frames.
			RECT z = convert(dpiToPx(currentDpi(), r));

			MoveWindow(h, z.left, z.top, z.right - z.left, z.bottom - z.top, TRUE);
		}
	}

	void Window::focus() {
		if (created())
			SetFocus(handle().hwnd());
		else if (Frame *f = rootFrame())
			f->focus(this);
	}

	void Window::font(Font *font) {
		if (font != myFont)
			myFont = new (this) Font(*font);
		if (created())
			SendMessage(handle().hwnd(), WM_SETFONT, (WPARAM)font->handle(currentDpi()), TRUE);
	}

	void Window::update() {
		if (created())
			UpdateWindow(handle().hwnd());
	}

	void Window::repaint() {
		if (created())
			InvalidateRect(handle().hwnd(), NULL, FALSE);
	}

	void Window::attachPainter() {
		if (!created())
			return;

		repaint();
	}

	void Window::detachPainter() {
		if (!created())
			return;

		// We must ask really hard to make Windows repaint the window properly.
		RedrawWindow(handle().hwnd(), NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
	}

	bool Window::onCommand(nat id) {
		return false;
	}

	bool Window::onNotify(NMHDR *data) {
		return false;
	}

	Nat Window::currentDpi() {
		if (myRoot)
			return myRoot->currentDpi();
		else
			return defaultDpi;
	}

	void Window::updateDpi(Bool move) {
		// Update our position.
		if (move)
			pos(myPos);

		// Update the font.
		if (myFont)
			font(myFont);
	}

	bool Window::create(ContainerBase *parent, nat id) {
		return createEx(NULL, childFlags, 0, parent->handle().hwnd(), id);
	}

	bool Window::createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent) {
		return createEx(className, style, exStyle, parent, 0, cNormal);
	}

	bool Window::createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent, nat id) {
		return createEx(className, style, exStyle, parent, id, cNormal);
	}

	bool Window::createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent, nat id, CreateFlags flags) {
		assert(handle() == invalid);

		App *app = gui::app(engine());

		if (className == null)
			className = (LPCTSTR)app->windowClass();

		if (cManualVisibility & ~flags) {
			if (myVisible) {
				style |= WS_VISIBLE;
			} else {
				style &= ~WS_VISIBLE;
			}
		}

		if (!myEnabled)
			style |= WS_DISABLED;

		Point p = myPos.p0;
		Size s = myPos.size();

		if (WS_CHILD & ~style) {
			RECT r = convert(myPos);
			AdjustWindowRectEx(&r, style & ~WS_OVERLAPPED, FALSE, exStyle);
			Rect c = convert(r);
			p = c.p0;
			s = c.size();
		} else if (parent) {
			Nat dpi = windowDpi(parent);
			p = p * dpiScale(dpi);
			s = s * dpiScale(dpi);
		}

		// Put everything in ints now, so that we can properly set CW_USEDEFAULT without potential
		// rounding errors.
		int x = (int)p.x;
		int y = (int)p.y;
		int width = (int)s.w;
		int height = (int)s.h;

		if (flags & cAutoPos) {
			x = CW_USEDEFAULT;
			y = 0;

			if (parent != NULL) {
				// Center the dialog over the parent.
				RECT parentRect;
				GetWindowRect(parent, &parentRect);

				x = int(parentRect.left + (parentRect.right - parentRect.left) / 2 - s.w / 2);
				y = int(parentRect.top + (parentRect.bottom - parentRect.top) / 2 - s.h / 2);
			}

			// Invalid size?
			if (!myPos.size().valid()) {
				width = CW_USEDEFAULT;
				height = 0;
			}
		}

		// Position controls before creation.
		if (myPos.size().valid())
			onResize(myPos.size());

		HINSTANCE instance = app->instance();
		LPCTSTR windowName = myText->toCrLf()->c_str();

		app->preCreate(this);
		HWND z = CreateWindowEx(exStyle, className, windowName, style,
								x, y, width, height,
								parent, (HMENU)id, instance, NULL);

		if (z == NULL) {
			app->createAborted(this);
			return false;
		}

		if (WS_CHILD & ~style) {
			// Take DPI into account.
			Nat dpi = windowDpi(z);
			if (dpi != defaultDpi) {
				RECT r = convert(Rect(Point(), myPos.size() * dpiScale(dpi)));
				dpiAdjustWindowRectEx(&r, style & ~WS_OVERLAPPED, FALSE, exStyle, dpi);
				s = convert(r).size();
				SetWindowPos(z, NULL, 0, 0, int(s.w), int(s.h), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}

		handle(z);
		if (timerInterval != Duration()) {
			setTimer(timerInterval);
		}
		SendMessage(handle().hwnd(), WM_SETFONT, (WPARAM)myFont->handle(currentDpi()), TRUE);
		return true;
	}

	MsgResult Window::onPaint() {
		if (myPainter) {
			RepaintParams p;
			myPainter->uiRepaint(&p);
			ValidateRect(handle().hwnd(), NULL);

			return msgResult(0);
		} else {
			return noResult();
		}
	}

	void Window::setTimer(Duration interval) {
		timerInterval = interval;
		if (created()) {
			// The HWND is used as a timer ID. That makes it unique. We can still use timer #0 for internal stuff.
			SetTimer(handle().hwnd(), (UINT_PTR)handle().hwnd(), (UINT)interval.inMs(), NULL);
		}
	}

	void Window::clearTimer() {
		if (handle() != Handle())
			KillTimer(handle().hwnd(), (UINT_PTR)handle().hwnd());
	}

#endif
#ifdef GUI_GTK

	class Timer {
	public:
		Timer(Duration interval, GtkWidget *widget, Engine &engine);
		~Timer();

	private:
		GtkWidget *widget;
		Engine *engine;
		guint id;

		static gboolean callback(gpointer user_data);
	};

	Timer::Timer(Duration interval, GtkWidget *widget, Engine &engine) :
		widget(widget), engine(&engine),
		id(g_timeout_add(interval.inMs(), &callback, this)) {}

	Timer::~Timer() {
		if (id) {
			GSource *s = g_main_context_find_source_by_id(NULL, id);
			if (s)
				g_source_destroy(s);

			id = 0;
		}
	}

	gboolean Timer::callback(gpointer user_data) {
		Timer *me = (Timer *)user_data;
		App *app = gui::app(*me->engine);

		Window *win = app->findWindow(Handle(me->widget));
		win->onTimer();

		return G_SOURCE_CONTINUE;
	}


	bool Window::create(ContainerBase *parent, nat id) {
		// Create an event box so that we receive events for this window.
		initWidget(parent, gtk_event_box_new());
		return true;
	}

	void Window::initWidget(ContainerBase *parent, GtkWidget *widget) {
		handle(widget);

		parent->addChild(widget, myPos);

		if (myVisible)
			gtk_widget_show(widget);
		else
			gtk_widget_hide(widget);

		if (!myEnabled)
			gtk_widget_set_sensitive(widget, FALSE);

		if (myFont != app(engine())->defaultFont)
			gtk_widget_override_font(fontWidget(), myFont->desc());

		initSignals(widget, drawWidget());
	}

	void Window::initSignals(GtkWidget *widget, GtkWidget *draw) {
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onKeyDown>::to(widget, "key-press-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onKeyUp>::to(widget, "key-release-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onButton>::to(widget, "button-press-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onButton>::to(widget, "button-release-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onMotion>::to(widget, "motion-notify-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onEnter>::to(widget, "enter-notify-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onLeave>::to(widget, "leave-notify-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onScroll>::to(widget, "scroll-event", engine());
		Signal<void, Window, GdkRectangle *>::Connect<&Window::onSize>::to(widget, "size-allocate", engine());
		Signal<void, Window>::Connect<&Window::onRealize>::to(draw, "realize", engine());
		Signal<void, Window>::Connect<&Window::onUnrealize>::to(draw, "unrealize", engine());
		Signal<gboolean, Window, cairo_t *>::Connect<&Window::onDraw>::to(draw, "draw", engine());

		if (timerInterval != Duration())
			setTimer(timerInterval);
	}

	void Window::destroyWindow(Handle handle) {
		gtk_widget_destroy(handle.widget());
		if (gTimer) {
			delete gTimer;
			gTimer = null;
		}
	}

	gboolean Window::onKeyUp(GdkEvent *event) {
		GdkEventKey &k = event->key;
		return onKey(false, keycode(k), modifiers(k)) ? TRUE : FALSE;
	}

	gboolean Window::onKeyDown(GdkEvent *event) {
		GdkEventKey &k = event->key;
		bool ok = onKey(true, keycode(k), modifiers(k));
		if (ok)
			ok = onChar(k.keyval);
		return ok ? TRUE : FALSE;
	}

	// Translate widget coordinates to window coordinates.
	static void translateToWindow(GtkWidget *widget, gdouble &x, gdouble &y) {
		if (!widget)
			return;

		// Note: This might not be entirely correct in nested structures.
		// Note: Seems to work OK inside a tab control, which seems to consist of multiple windows. Not entirely sure though.

		for (GtkWidget *at = widget; !gtk_widget_get_has_window(at); at = gtk_widget_get_parent(at)) {
			GtkAllocation delta;
			gtk_widget_get_allocation(at, &delta);

			x += delta.x;
			y += delta.y;
		}
	}

	// Translate window coordinates to widget coordinates. Note: Due to how Gtk propagates events,
	// it might be the case that 'eventWindow' is a child window to whatever window 'to' might be inside.
	static bool translatePoint(GdkWindow *eventWindow, GtkWidget *to, gdouble x, gdouble y, Point &out) {
		// Find a widget that has a window by traversing from 'to'.
		GtkWidget *toParent = to;
		while (!gtk_widget_get_has_window(toParent)) {
			toParent = gtk_widget_get_parent(toParent);

			if (!toParent) {
				WARNING(L"Failed to find a parent widget with a window!");
				return false;
			}
		}

		GdkWindow *toWindow = gtk_widget_get_window(toParent);

		// At this point, we know that 'toParent' is a parent of 'eventWindow'. Try to find it by
		// traversing from 'eventWindow', and updating coordinates as we go. GdkWindow coordinates
		// are relative to the parent window, in contrast to widgets.
		while (eventWindow != toWindow) {
			// Make coordinates relative the window's parent.
			gint winX, winY;
			gdk_window_get_position(eventWindow, &winX, &winY);
			x += winX;
			y += winY;

			// Traverse.
			eventWindow = gdk_window_get_effective_parent(eventWindow);

			if (!eventWindow) {
				// This happens as it seems like events for a window are passed to the dialog window
				// first. As such, we should not warn in this case.
				return false;
			}
		}

		// Now, we have coordinates that are relative to the window owned by "toParent". Now we can
		// just ask Gtk to convert coordinates for us!
		gint xDelta, yDelta;
		gtk_widget_translate_coordinates(toParent, to, 0, 0, &xDelta, &yDelta);
		x += xDelta;
		y += yDelta;

		// Finally, we can ask Gtk about the allocation of 'to' and make sure that the coordinate is
		// inside the rectangle.
		GtkAllocation toAlloc;
		gtk_widget_get_allocation(to, &toAlloc);

		out.x = x;
		out.y = y;

		return out.x >= 0.0f && out.y >= 0.0f
			&& out.x < toAlloc.width && out.y < toAlloc.height;
	}

	gboolean Window::onButton(GdkEvent *event) {
		Point pt;
		GdkEventButton &b = event->button;
		if (!translatePoint(b.window, drawWidget(), b.x, b.y, pt))
			return FALSE;

		mouse::MouseButton button = mouse::MouseButton(b.button - 1);
		bool ok = false;

		if (button == mouse::left || button == mouse::middle || button == mouse::right) {
			switch (b.type) {
			case GDK_BUTTON_PRESS:
				ok = onClick(true, pt, button);
				break;
			case GDK_BUTTON_RELEASE:
				ok = onClick(false, pt, button);
				break;
			case GDK_2BUTTON_PRESS:
				ok = onDblClick(pt, button);
				break;
			}
		}
		return ok ? TRUE : FALSE;
	}

	gboolean Window::onMotion(GdkEvent *event) {
		Point pt;
		GdkEventMotion &m = event->motion;
		if (!translatePoint(m.window, drawWidget(), m.x, m.y, pt))
			return FALSE;

		return onMouseMove(pt) ? TRUE : FALSE;
	}

	gboolean Window::onEnter(GdkEvent *) {
		onMouseEnter();

		// No propagation.
		return TRUE;
	}

	gboolean Window::onLeave(GdkEvent *) {
		onMouseLeave();

		// No propagation.
		return TRUE;
	}

	gboolean Window::onScroll(GdkEvent *event) {
		Point pt;
		GdkEventScroll &s = event->scroll;
		if (!translatePoint(s.window, drawWidget(), s.x, s.y, pt)) {
			return FALSE;
		}

		const Int step = 120;

		bool ok = false;
		GdkScrollDirection direction;
		gdouble dx = 0, dy = 0;
		if (gdk_event_get_scroll_direction(event, &direction)) {
			switch (s.direction) {
			case GDK_SCROLL_UP:
				ok = onMouseVScroll(pt, -step);
				break;
			case GDK_SCROLL_DOWN:
				ok = onMouseVScroll(pt, step);
				break;
			case GDK_SCROLL_LEFT:
				ok = onMouseHScroll(pt, -step);
				break;
			case GDK_SCROLL_RIGHT:
				ok = onMouseHScroll(pt, step);
				break;
			}
		} else if (gdk_event_get_scroll_deltas(event, &dx, &dy)) {
			Int x(dx * step);
			if (x != 0)
				ok |= onMouseHScroll(pt, -x);
			Int y(dy * step);
			if (y != 0)
				ok |= onMouseVScroll(pt, -y);
		}
		return ok ? TRUE : FALSE;
	}

	void Window::onSize(GdkRectangle *alloc) {
		// Note: We're interested in forwarding the size of the drawing widget (if it differs from
		// the root-widget of the window).
		GtkWidget *draw = drawWidget();
		GtkAllocation drawAlloc;
		gtk_widget_get_allocation(draw, &drawAlloc);

		// If we have our own window, resize that as well.
		if (gdkWindow) {
			// Modify the x and y coords if we're several layers down.
			gdouble x = drawAlloc.x, y = drawAlloc.y;
			translateToWindow(gtk_widget_get_parent(draw), x, y);

			// gdk_window_move_resize(gdkWindow, alloc->x, alloc->y, alloc->width, alloc->height);
			gdk_window_move_resize(gdkWindow, x, y, drawAlloc.width, drawAlloc.height);
		}

		Size s(drawAlloc.width, drawAlloc.height);
		myPos.size(s);

		// GTK sends us this signal quite often, so filter out dummy draws.
		if (lastWidth != drawAlloc.width || lastHeight != drawAlloc.height) {
			lastWidth = drawAlloc.width;
			lastHeight = drawAlloc.height;

			if (myPainter) {
				int scale = gtk_widget_get_scale_factor(draw);
				myPainter->uiResize(s * scale, scale);
			}
			onResize(s);

			// Queue a repaint for this window.
			gtk_widget_queue_draw(handle().widget());

			// // Queue a repaint for this window. Otherwise, resizing windows that partially contain some
			// // continuous rendering will not always be updated properly. We're probably in a race with
			// // Gtk when we just update a part of the window.
			// GdkWindow *window = gtk_widget_get_window(handle().widget());
			// if (window) {
			// 	gdk_window_invalidate_rect(window, NULL, false);
			// }
		}
	}

	bool Window::preExpose(GtkWidget *widget) {
		// Do we have a painter?
		if (myPainter && gdkWindow && widget == drawWidget()) {
			// Does not seem to be needed right now, but this is where one can hook into paint
			// events before Gtk+ gets hold of them!

			// RepaintParams params = { gdkWindow, drawWidget(), NULL };
			// myPainter->uiRepaint(&params);
			// return true;
		}

		// Process normally.
		return false;
	}

	gboolean Window::onDraw(cairo_t *ctx) {
		GdkWindow *window = gdkWindow;
		if (!window)
			window = gtk_widget_get_window(drawWidget());

		// Anything we should do at all?
		if (!window)
			return FALSE;

		// Is this draw event for us?
		if (!gtk_cairo_should_draw_window(ctx, window))
			return FALSE;

		// We will call ourselves in some cases.
		if (drawing)
			return FALSE;

		// Do we have a painter?
		if (myPainter) {
			RepaintParams params = { window, drawWidget(), ctx };
			myPainter->uiRepaint(&params);

			// If we're using a native window, we need to inhibit the default behavior.
			if (gdkWindow)
				return TRUE;

			return FALSE;

			//  This is not needed anymore. We're being kind to Gtk+ and are repainting through the
			//  provided cairo_t context.
			// if (GDK_IS_WAYLAND_WINDOW(window)) {
			// 	// This is a variant of what we're doing below.
			// 	GtkWidget *me = drawWidget();
			// 	GtkWidget *parentWidget = gtk_widget_get_parent(me);
			// 	if (!parentWidget || !GTK_IS_CONTAINER(parentWidget)) {
			// 		// If no parent exists, or the parent is not a container, we simply delegate to the
			// 		// default behaviour. This should not happen in practice, but it is good to have a
			// 		// decent fallback. Since the parent is not a container, the default implementation
			// 		// will probably work anyway.
			// 		return false;
			// 	}

			// 	GtkContainer *parent = GTK_CONTAINER(gtk_widget_get_parent(me));

			// 	// It seems Gtk+ does not understand what we're doing and therefore miscalculates the x-
			// 	// and y- positions of the layout during drawing. We correct this by using the offset in
			// 	// the cairo_t.
			// 	GtkAllocation position;
			// 	gtk_widget_get_allocation(me, &position);

			// 	// Since we have to pretend that the current child does not have a window when calling
			// 	// 'gtk_container_propagate_draw' below, the current widget's offset will be applied twice.
			// 	cairo_translate(ctx, -position.x, -position.y);

			// 	drawing = true;
			// 	gtk_container_propagate_draw(parent, me, ctx);
			// 	drawing = false;
			// }

			// return TRUE;
		}

		// Do we have our own window and need to paint the background?
		if (!gdkWindow)
			return FALSE;

		// Do we have a window and need to paint the background?
		GtkWidget *me = drawWidget();
		GtkWidget *parentWidget = gtk_widget_get_parent(me);
		if (!parentWidget || !GTK_IS_CONTAINER(parentWidget)) {
			// If no parent exists, or the parent is not a container, we simply delegate to the
			// default behaviour. This should not happen in practice, but it is good to have a
			// decent fallback. Since the parent is not a container, the default implementation
			// will probably work anyway.
			return FALSE;
		}
		GtkContainer *parent = GTK_CONTAINER(gtk_widget_get_parent(me));


		// Fill with the background color we figured out earlier.
		App *app = gui::app(engine());
		Color bg = app->defaultBgColor;
		cairo_set_source_rgba(ctx, bg.r, bg.g, bg.b, bg.a);
		cairo_paint(ctx);

		// It seems Gtk+ does not understand what we're doing and therefore miscalculates the x-
		// and y- positions of the layout during drawing. We correct this by using the offset in
		// the cairo_t.
		GtkAllocation position;
		gtk_widget_get_allocation(me, &position);

		// Since we have to pretend that the current child does not have a window when calling
		// 'gtk_container_propagate_draw' below, the current widget's offset will be applied twice.
		cairo_translate(ctx, -position.x*2, -position.y*2);

		// Call the original draw function here, with our translated context.
		// typedef gboolean (*DrawFn)(GtkWidget *, cairo_t *);
		// DrawFn original = GTK_WIDGET_GET_CLASS(me)->draw;
		// if (original)
		// 	(*original)(me, ctx);

		// Call 'gtk_container_propagate_draw' to draw ourselves again. Remember that we are
		// currently drawing so that we do not get into an infinite recursion when we are called
		// again. We could call 'GTK_WIDGET_GET_CLASS(me)->draw' instead, but that function
		// assumes that the cairo_t is properly transformed so that (0, 0) is at the top left of
		// the widget. That is not the case now, due to the possible bug inside
		// 'gtk_container_propagate_draw'.
		gtk_widget_set_has_window(me, FALSE);
		drawing = true;
		gtk_container_propagate_draw(parent, me, ctx);
		drawing = false;
		gtk_widget_set_has_window(me, TRUE);

		return TRUE;
	}

	bool Window::useNativeWindow() {
		// By default, we don't need separate windows in the rendering model. This might, however,
		// be expensive in cases where many widgets are overlaid the rendering area. For these
		// cases, we do allow creating separate X windows by setting an environment variable.


		// On Wayland, we don't want a separate window as it confuses the window manager.
		if (GDK_IS_WAYLAND_WINDOW(gtk_widget_get_window(drawWidget())))
			return false;

		// Initialize the variable once. This is atomic and will only be executed once.
		static const char *RENDER_X_WINDOW = getenv(ENV_RENDER_X_WINDOW);

		// Check the environment variable.
		if (RENDER_X_WINDOW) {
			// It exists! We need to create a separate window in two cases:
			// 1: we have a painter attached
			// 2: our parent has a painter attached
			if (myPainter)
				return true;
			if (myParent && myParent != this && myParent->myPainter)
				return true;
		}

		return false;
	}

	void Window::onRealize() {
		if (gdkWindow)
			return;

		if (!useNativeWindow()) {
			setWindowMask(gtk_widget_get_window(drawWidget()));

			// Make sure we are using double buffering when drawing to it in "composite" mode.
			gtk_widget_set_double_buffered(drawWidget(), TRUE);
			if (myPainter)
				myPainter->uiAttach(this);
			return;
		}

		// Create the window.
		GdkWindowAttr attrs;
		memset(&attrs, 0, sizeof(attrs));

		GtkWidget *drawTo = drawWidget();

		GdkWindow *oldWindow = gtk_widget_get_window(drawTo);
		GdkWindow *parentWindow = gtk_widget_get_parent_window(drawTo);
		if (oldWindow && parentWindow != oldWindow) {
			WARNING(L"Using previously created windows could be bad.");
			// This widget already has its own window. Use that.
			gdkWindow = oldWindow;
			if (myPainter)
				gtk_widget_set_double_buffered(drawTo, FALSE);
			if (myPainter)
				myPainter->uiAttach(this);
			return;
		}
		if (!oldWindow)
			oldWindow = parentWindow;

		GtkAllocation alloc;
		gtk_widget_get_allocation(drawTo, &alloc);
		gdouble x = 0, y = 0;
		translateToWindow(drawTo, x, y);
		attrs.x = x;
		attrs.y = y;
		attrs.width = alloc.width;
		attrs.height = alloc.height;
		attrs.event_mask = gtk_widget_get_events(drawTo) | GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
		attrs.window_type = GDK_WINDOW_CHILD; // GDK_WINDOW_SUBSURFACE is nice on Wayland.
		attrs.wclass = GDK_INPUT_OUTPUT;
		// Probably good. We could use *_get_system_visual() instead.
		attrs.visual = gdk_screen_get_rgba_visual(gdk_window_get_screen(oldWindow));

		gdkWindow = gdk_window_new(oldWindow, &attrs, GDK_WA_X | GDK_WA_Y);
		gdk_window_ensure_native(gdkWindow);
		gdk_window_show_unraised(gdkWindow);

		// Unref the old window.
		if (oldWindow)
			g_object_unref(oldWindow);

		gtk_widget_set_has_window(drawTo, TRUE);
		gtk_widget_set_window(drawTo, gdkWindow);
		gdk_window_set_user_data(gdkWindow, drawTo);
		if (myPainter) {
			gtk_widget_set_double_buffered(drawTo, FALSE);
		}
		setWindowMask(gdkWindow);
		if (myPainter)
			myPainter->uiAttach(this);
	}

	void Window::onUnrealize() {
		if (!gdkWindow)
			return;

		GtkWidget *drawTo = drawWidget();
		// gtk_widget_set_window(drawTo, NULL);
		gtk_widget_set_has_window(drawTo, FALSE);

		if (myPainter)
			myPainter->uiDetach();
		gdkWindow = null;
	}

	void Window::setWindowMask(GdkWindow *window) {
		if (window) {
			int mask = gdk_window_get_events(window);
			mask |= GDK_POINTER_MOTION_MASK;
			mask |= GDK_BUTTON_MOTION_MASK;
			mask |= GDK_BUTTON_PRESS_MASK;
			mask |= GDK_BUTTON_RELEASE_MASK;
			mask |= GDK_SCROLL_MASK;
			mask |= GDK_SMOOTH_SCROLL_MASK;
			mask |= GDK_LEAVE_NOTIFY_MASK;
			gdk_window_set_events(window, GdkEventMask(mask));
		}
	}

	static void recreate_widget(GtkWidget *widget) {
		gtk_widget_unmap(widget);
		gtk_widget_unrealize(widget);

		gtk_widget_realize(widget);
		gtk_widget_map(widget);
	}

	void Window::attachPainter() {
		if (!created())
			return;

		if (gdkWindow)
			return;

		recreate_widget(drawWidget());
	}

	void Window::detachPainter() {
		if (!created())
			return;

		if (!gdkWindow)
			return;

		recreate_widget(drawWidget());
	}

	Str *Window::text() {
		return myText;
	}

	void Window::text(Str *str) {
		myText = str;
	}

	Rect Window::pos() {
		return myPos;
	}

	void Window::pos(Rect r) {
		myPos = r;
		if (created() && parent()) {
			parent()->moveChild(handle().widget(), myPos);
		}
	}

	void Window::visible(Bool v) {
		myVisible = v;
		if (created()) {
			if (v)
				gtk_widget_show(handle().widget());
			else
				gtk_widget_hide(handle().widget());
		}
	}

	void Window::enabled(Bool v) {
		myEnabled = v;
		if (created()) {
			gtk_widget_set_sensitive(handle().widget(), v ? TRUE : FALSE);
		}
	}

	void Window::focus() {
		if (created())
			gtk_widget_grab_focus(handle().widget());
		else if (Frame *f = rootFrame())
			f->focus(this);
	}

	void Window::font(Font *font) {
		myFont = new (this) Font(*font);
		if (created())
			gtk_widget_override_font(fontWidget(), myFont->desc());
	}

	GtkWidget *Window::fontWidget() {
		return handle().widget();
	}

	GtkWidget *Window::drawWidget() {
		return handle().widget();
	}

	void Window::update() {
		if (created()) {
			gtk_widget_queue_draw(handle().widget());
			// GdkWindow *window = gtk_widget_get_window(handle().widget());
			// if (window) {
			// 	gdk_window_invalidate_rect(window, NULL, true);
			// 	gdk_window_process_updates(window, true);
			// }
		}
	}

	void Window::repaint() {
		if (created()) {
			gtk_widget_queue_draw(handle().widget());
			// GdkWindow *window = gtk_widget_get_window(handle().widget());
			// if (window)
			// 	gdk_window_invalidate_rect(window, NULL, true);
		}
	}

	void Window::setTimer(Duration interval) {
		timerInterval = interval;
		if (created()) {
			delete gTimer;
			gTimer = null;

			if (interval == Duration())
				return;

			gTimer = new Timer(interval, handle().widget(), engine());
		}
	}

	void Window::clearTimer() {
		if (gTimer)
			delete gTimer;
		gTimer = null;
	}

#endif

}
