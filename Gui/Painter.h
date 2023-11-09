#pragma once
#include "Handle.h"
#include "Window.h"
#include "WindowGraphics.h"
#include "Surface.h"
#include "RenderMgr.h"
#include "Core/WeakSet.h"
#include "Core/Lock.h"

namespace gui {
	class App;


	/**
	 * Object responsible for painting window contents. Create a subclass and override
	 * 'render'. Then attach it to a Window.
	 */
	class Painter : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		STORM_CTOR Painter();

		virtual ~Painter();

		// Called to render the window. Before this call, the output is filled with `bgColor`.
		// `size` is the drawing size in device independent units.
		// Return `true` to schedule a repaint for the next frame.
		virtual Bool STORM_FN render(Size size, Graphics *graphics);

		// Called from Storm to schedule a repaint of the window.
		virtual void STORM_FN repaint();

		// Background color. Updated on next redraw.
		Color bgColor;


		/**
		 * The following functions (the ones starting with 'uiXxx') are intended to be called
		 * directly from Window on the Ui thread (not the Render thread). This is for two
		 * reasons. Firstly, convenience inside the Window class. But more importantly, it is since
		 * some backends require to do some work on the Ui thread to synchronize drawing.
		 */

		// Attach to a window.
		void uiAttach(Window *to);

		// Detach from the currently attached window.
		void uiDetach();

		// Called when the attached window has been resized. "scale" is used to scale the initial
		// layer of the canvas in case we're running in a non-default DPI mode.
		void uiResize(Size size, Float scale);

		// Called when the attached window wants to be repainted. The parameter passed contains
		// OS-specific data.
		void uiRepaint(RepaintParams *params);

	private:
		friend class RenderMgr;

		// Attached HWND.
		Handle attachedTo;

		// Surface to render to.
		Surface *surface;

		// Graphics object. Associated to the surface.
		WindowGraphics *graphics;

		// App object.
		App *app;

		// Render manager.
		RenderMgr *mgr;

		// Lock for the target surface.
		Lock *lock;

		// Registered for continuous repaints in RenderMgr?  If true, then calls to 'repaint' will
		// not do anything, instead we rely on RenderMgr to repaint us every frame.
		Bool continuous;

		// Are we registered with continuous repaints with the windowing system. This is essentially
		// a copy of "continuous", but it is updated in response to repaint messages on the Ui
		// thread rather than on the Render thread.
		Bool uiContinuous;

		// Do we need to wait for "paint" messages from the window system?
		Bool synchronizedPresent;

		// Were we resized recently? If so, we don't have any frame available.
		Bool resized;

		// Are we currently rendering? Used to prevent recursive entries to rendering on the same thread.
		Bool currentRendering;

		// Are we attached to anything? If we get detached while waiting to render the screen, we
		// need to be able to detect that.
		inline bool attached() const { return graphics != null; }

		// Ready to render? Called by the render manager to determine if the painter is ready to
		// render when it is in continuous mode. Some implementations may need to wait for some
		// event even when they are in continuous mode. If a painter ever returns false from
		// ready(), it needs to call 'RenderMgr::painterReady' when it becomes ready again.
		bool ready();

		// Destroy any resources connected to the current device.
		void destroy();

		// Repaint number (to keep track of when 'redraw' should return and avoid flicker).
		volatile Nat repaintCounter;

		// Last repaint shown on the screen. Used to determine if we are ready to draw the next frame.
		volatile Nat currentRepaint;

		// Callback ID used for the tick callback in Gtk. Not used on Windows.
		Nat tickCallbackId;

		// Attach to a specific window.
		void CODECALL attach(Window *to);

		// Detach from the previous window.
		void CODECALL detach();

		// Called when the attached window has been resized.
		void CODECALL resize(Size to, Float scale);

		// Called when the attached window wants to be repainted. The parameter passed is
		// OS-specific data.
		void CODECALL repaintI(RepaintParams *params);

		// Called from the UI thread.
		void uiAfterRepaint(RepaintParams *handle);

		// Wait for a new frame to be rendered (used during continuous updates).
		void waitForFrame();

		// Do repaints (always).
		void doRepaint(bool waitForVSync, bool fromDraw);

		// Do repaints without having to worry about scheduling repaints.
		bool doRepaintI(bool waitForVSync, bool fromDraw);

		// Ask the attached window to repaint itself whenever convenient.
		void repaintAttachedWindow();

		// Called when the window system should be asked for continuous repaints.
		void uiSetContinuous(bool continuous);
	};

}
