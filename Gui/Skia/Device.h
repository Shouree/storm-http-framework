#pragma once
#include "Gui/Device.h"
#include "Gui/Surface.h"
#include "Skia.h"

#ifdef GUI_ENABLE_SKIA

namespace gui {

	class SkiaDevice;

	/**
	 * Rendering context for Skia.
	 */
	class SkiaContext : public GLDevice::Context {
	public:
		// Create.
		SkiaContext(SkiaDevice *owner, GdkWindow *window, GdkGLContext *context);

		// Destroy.
		~SkiaContext();

		// Skia context.
		sk_sp<GrDirectContext> skia;

		// Make sure this context is active. Skia does not seem to switch contexts itself, so we
		// will need to do it for Skia. As such, we try to minimize calls to "gl_area_make_current".
		void makeCurrent();

		// Clear which context is the current one, in case someone manually modifies the current
		// context.
		void clearCurrent();

	private:
		// Pointer to the device, so that we may know which context is the current one.
		SkiaDevice *device;
	};

	/**
	 * A Skia device.
	 */
	class SkiaDevice : public GLDevice {
		friend class SkiaContext;
	public:
		// Create.
		SkiaDevice(Engine &e);

		// Create a text manager.
		virtual TextMgr *createTextMgr();

	protected:
		// Create a context.
		virtual SkiaContext *createContext(GdkWindow *window, GdkGLContext *context);

		// Create a surface.
		virtual Surface *createSurface(GtkWidget *widget, Context *context);

	private:
		// Current active context. To skip calls to make_current.
		SkiaContext *current;
	};


	/**
	 * A Skia surface.
	 */
	class SkiaSurface : public Surface {
	public:
		// Create.
		SkiaSurface(Size size, Float scale, SkiaContext *context);

		// Destroy.
		~SkiaSurface();

		// Present.
		virtual PresentStatus present(bool waitForVSync);

		// Paint.
		virtual void repaint(RepaintParams *params);

		// Resize the surface.
		virtual void resize(Size size, Float scale);

		// Create a Graphics object.
		virtual WindowGraphics *createGraphics(Engine &e);

		// Canvas for the surface (a mirror of surface->getCanvas()). Updated whenever needed.
		SkCanvas *canvas;

		// Skia surface.
		sk_sp<SkSurface> surface;

		// Get the device.
		GrDirectContext *device() {
			return context->skia.get();
		}

		// Ensure that the surface is active.
		void makeCurrent() { context->makeCurrent(); }

	private:
		// Context.
		SkiaContext *context;

		// Frame buffer object.
		GLuint framebuffer;

		// Color buffer we're rendering to.
		GLuint colorbuffer;

		// Stencil buffer.
		GLuint stencilbuffer;

		// Backend render target. Must be kept alive as long as the surface is alive.
		GrBackendRenderTarget target;
	};

}

#endif
