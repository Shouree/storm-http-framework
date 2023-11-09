#pragma once
#include "Message.h"
#include "Handle.h"
#include "Font.h"
#include "Key.h"
#include "Mouse.h"
#include "Core/Timing.h"
#include "Core/TObject.h"
#include "Utils/Bitmask.h"

namespace gui {
	class ContainerBase;
	class Frame;
	class Painter;
	class Timer;

	// Modifiers for the create function.
	enum CreateFlags {
		cNormal = 0x0,
		cManualVisibility = 0x1,
		cAutoPos = 0x2,
	};

	BITMASK_OPERATORS(CreateFlags);


	/**
	 * Base class for windows and controls.
	 *
	 * The Window class itself creates a plain, empty window that can be used for drawing custom
	 * controls. The underlying window is not created directly, instead it is delayed until the
	 * window has been attached to a parent of some sort. The exception from this rule are Frames,
	 * which has no parents and therefore require special attention (see documentation for Frame).
	 *
	 * The fact that windows are not created until attached are hidden as good as possible. This
	 * means that we have to cache all properties as good as possible until the window is actually
	 * created.
	 *
	 * All windows have exactly one parent, except for frames which have none (technically their
	 * parent are the desktop, but we do not implement that here). Re-parenting of windows is not
	 * supported. The 'root' member of a Frame is set to the object itself.
	 */
	class Window : public ObjectOn<Ui> {
		STORM_CLASS;
	public:
		// Create an empty window.
		STORM_CTOR Window();

		// Destroy.
		virtual ~Window();

		// Invalid window handle.
		static const Handle invalid;

		// Get our handle. May be 'invalid'.
		Handle handle() const;

		// Set our handle, our parent is inferred from the handle itself.
		void handle(Handle handle);

		// Are we created?
		bool created() const { return handle() != invalid; }

		// Get the parent window. Null for frames.
		MAYBE(ContainerBase *) STORM_FN parent();

		// Get the frame that is in the root of the hierarchy. Returns ourself for frames, returns `null` before attached.
		MAYBE(Frame *) STORM_FN rootFrame();

		// Attach to a parent container and creation. To be called from 'container'.
		void attachParent(ContainerBase *parent);

		// Detach from our parent. This destroys the window.
		void detachParent();

		// Called when this window has been destroyed. This function is intended as a notification
		// for child classes in C++ (eg. Container). If you want to be notified when the window is
		// about to be destroyed, handle WM_CLOSE instead.
		virtual void windowDestroyed();

		// Note: 'parent' need to be set before calling this function. This initializes the creation of our window.
		virtual void parentCreated(nat id);

#ifdef GUI_WIN32
		// Called when a regular message has been received. If no result is returned, calls the
		// default window proc, or any message procedure declared by the window we're handling. Only
		// works for windows with the default window class provided by App. Otherwise, use 'preTranslateMessage'.
		virtual MsgResult onMessage(const Message &msg);

		// Called before a message is dispatched to the window procedure. Return a result to inhibit the regular
		// dispatch.
		virtual MsgResult beforeMessage(const Message &msg);

		// Called when a WM_COMMAND has been sent to (by) us. Return 'true' if it is handled. Type
		// is the notification code specified by the message (eg BN_CLICK).
		virtual bool onCommand(nat type);

		// Called when a WM_NOTIFY has been sent to (by) us. Return 'true' if it is handled. The
		// parameter is the header for notify as sent in the message. This can be casted to a
		// relevant struct to receive more information.
		virtual bool onNotify(NMHDR *data);

		// Get the current DPI for this window. Only makes sense on Windows.
		virtual Nat currentDpi();

		// Update this window to new DPI settings (i.e., "currentDpi()" has changed).
		virtual void updateDpi(Bool move);
#endif
#ifdef GUI_GTK
		// Called when we received an EXPOSE event, but before Gtk+ is informed about the
		// event. Return 'true' to inhibit the Gtk+ behaviour completely.
		bool preExpose(GtkWidget *widget);

#endif

		// Check if the window is visible.
		Bool STORM_FN visible();

		// Set if the window is visible or not. Assignment function, so `visible` can be used as a member variable.
		void STORM_ASSIGN visible(Bool show);

		// Check if the window is enabled.
		Bool STORM_FN enabled();

		// Set if the window is enabled. Generally only interesting to do for leaf windows.
		// Assignment function, so it can be used as a member variable.
		void STORM_ASSIGN enabled(Bool enable);

		// Get the window text. How this text is used depends on the window.
		virtual Str *STORM_FN text();

		// Set the window text. Assignment function, so it can be used as a member variable.
		virtual void STORM_ASSIGN text(Str *str);

		// Get the window position. Always relative to the client area of the parent (even in Frames).
		virtual Rect STORM_FN pos();

		// Set the window position.
		virtual void STORM_ASSIGN pos(Rect r);

		// Get the minimum size for this window. Note: This does not consider the size and position
		// of any child windows in case this is a container. This function is mostly useful for
		// determining the preferred size for various windows.
		virtual Size STORM_FN minSize();

		// Get the current font.
		Font *STORM_FN font();

		// Set the current font. Assignment function, so it can be used as a member variable.
		void STORM_ASSIGN font(Font *font);

		// Set focus to this window.
		virtual void STORM_FN focus();

		// Update the window (ie repaint it) right now.
		virtual void STORM_FN update();

		// Repaint the window when we have time.
		virtual void STORM_FN repaint();

		// Called when the window is resized.
		virtual void STORM_FN resized(Size size);

		// Convenience function to trigger uptade of layouts easily.
		void STORM_FN resized();

		// Called when a key is pressed or released (depending on `pressed`). Should return `true`
		// if the keypress was recognized and should not propagate further.
		virtual Bool STORM_FN onKey(Bool pressed, key::Key keycode, mod::Modifiers modifiers);

		// Called when a key has been typed. Note that this is sometimes different from a key press
		// event. A typical example is a dead key. The dead key will produce `onKey` events as
		// usual, but will not produce a `onChar` event until the next keypress, when it might be
		// combined with the next character. Another example is when using an IME.
		virtual Bool STORM_FN onChar(Nat charCode);

		// Called when a mouse button has been pressed or released (depending on `pressed`) at the
		// location `at` in the window.
		virtual Bool STORM_FN onClick(Bool pressed, Point at, mouse::MouseButton button);

		// Called when a mouse button has been double-clicked in the window.
		virtual Bool STORM_FN onDblClick(Point at, mouse::MouseButton button);

		// Called whenever the mouse is moved in the window.
		virtual Bool STORM_FN onMouseMove(Point at);

		// Called when the mouse has been scrolled in the vertical direction.
		virtual Bool STORM_FN onMouseVScroll(Point at, Int delta);

		// Called when the mouse has been scrolled in the horizontal direction.
		virtual Bool STORM_FN onMouseHScroll(Point at, Int delta);

		// Called when the mouse entered this window. The mouse is considered to be inside the
		// window only for as long as the mouse is directly on top of the window. For example, if
		// the mouse moves over a child window, the mouse is no longer considered to be inside of
		// this window.
		virtual void STORM_FN onMouseEnter();

		// Called when the mouse has left this window.
		virtual void STORM_FN onMouseLeave();

		// Set window contents (custom drawing).
		void STORM_ASSIGN painter(MAYBE(Painter *) to);

		// Get the current painter.
		MAYBE(Painter *) STORM_FN painter();

		// Called regularly if the `setTimer` has been called to start the timer.
		virtual void STORM_FN onTimer();

		// Set the window timer. Causes `onTimer` to be called regularly.
		void STORM_FN setTimer(Duration interval);

		// Stop timer.
		void STORM_FN clearTimer();

	protected:
		// Override this to do any special window creation. The default implementation creates a
		// plain child window with no window class. Called as soon as we know our parent (not on
		// Frames). Returns 'false' on failure.
		virtual bool create(ContainerBase *parent, nat id);

		// Internal 'resized' notification.
		virtual void onResize(Size size);

		// Destroy a handle.
		virtual void destroyWindow(Handle handle);

#ifdef GUI_WIN32
		// Create a window, and handle it. Makes sure that all messages are handled correctly.
		// Equivalent to handle(CreateWindowEx(...)), but ensures that any messages sent before
		// CreateWindowEx returns are sent to this class as well.
		// If NULL is passed as the class name, the default window class will be used.
		bool createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent, nat id, CreateFlags flags);
		bool createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent, nat id);
		bool createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent);
#endif
#ifdef GUI_GTK
		// Called to perform initialization of the recently created widget. Performs things such as
		// setting visibility, text and position. Also calls 'handle()' on the widget.
		void initWidget(ContainerBase *parent, GtkWidget *widget);

		// Initialize any signals required by the Window class.
		virtual void initSignals(GtkWidget *widget, GtkWidget *draw);

		// Get the widget we shall set the font on.
		virtual GtkWidget *fontWidget();

	public:
		// Get the widget we shall draw on.
		virtual GtkWidget *drawWidget();
#endif
	private:
		// Handle.
		Handle myHandle;

		// Parent.
		ContainerBase *myParent;

		// Root.
		Frame *myRoot;

		// Visible?
		Bool myVisible;

		// Enabled.
		Bool myEnabled;

		// Currently drawing anything to this window?
		Bool drawing;

		// Is the mouse inside this window? (Used on Win32 to implement enter/leave notifications)
		Bool mouseInside;

		// Text.
		Str *myText;

	protected:
		// Position (Frame needs to modify this).
		Rect myPos;

	private:
		// Font.
		Font *myFont;

		// Painter.
		Painter *myPainter;

		// Last allocated width and height. Used to filter out duplicate allocate messages.
		int lastWidth;
		int lastHeight;

		// In Gtk+, widgets are usually not rendered in separate windows. When we're using OpenGL
		// rendering, we need separate windows for the widget being drawn to, and any child
		// widgets. This variable represents the created window.
		UNKNOWN(PTR_NOGC) GdkWindow *gdkWindow;

		// In Gtk+, we need to allocate a timer object separatly.
		UNKNOWN(PTR_NOGC) Timer *gTimer;

		// Timer timeout (nonzero = set).
		Duration timerInterval;

		// Prepare for a painter/prepare for no painter. Not called when we swap painter.
		void attachPainter();
		void detachPainter();

#ifdef GUI_WIN32
		// Handle on paint events.
		MsgResult onPaint();
#endif
#ifdef GUI_GTK
		// Signal landing pads.
		gboolean onKeyUp(GdkEvent *event);
		gboolean onKeyDown(GdkEvent *event);
		gboolean onButton(GdkEvent *event);
		gboolean onMotion(GdkEvent *event);
		gboolean onEnter(GdkEvent *event);
		gboolean onLeave(GdkEvent *event);
		gboolean onScroll(GdkEvent *event);
		void onSize(GdkRectangle *alloc);
		gboolean onDraw(cairo_t *ctx);
		void onRealize();
		void onUnrealize();

		// Set window mask.
		void setWindowMask(GdkWindow *window);

		// Do we need to be a separate native window? Called during realization.
		bool useNativeWindow();
#endif
	};

}
