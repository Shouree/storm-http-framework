#include "stdafx.h"
#include "Device.h"
#include "App.h"
#include "Core/Convert.h"
#include "D2D/Device.h"
#include "Cairo/Device.h"
#include "Cairo/GLDevice.h"
#include "Skia/Device.h"
#include "Exception.h"
#include "Window.h"
#include "RenderMgr.h"
#include "Workaround/Apply.h"

namespace gui {

#if defined(GUI_WIN32)

	Device *Device::create(Engine &e) {
		return applyEnvWorkarounds(new D2DDevice(e));
	}

#elif defined(GUI_GTK)

	GtkWidget *Device::drawWidget(Engine &e, Handle handle) {
		Window *w = app(e)->findWindow(handle);
		if (!w)
			return handle.widget();
		else
			return w->drawWidget();
	}

	template <class T>
	Device *create(Engine &e) {
		return new T(e);
	}

	struct CreateDevice {
		const char *name;
		Device *(*create)(Engine &e);
	};

	// These are ordered by priority:
	static CreateDevice devices[] = {
#ifdef GUI_ENABLE_SKIA
		{ "skia", &create<SkiaDevice> },
#endif
#ifdef GUI_ENABLE_CAIRO_GL
		{ "gl", &create<CairoGLDevice> },
#endif
		{ "gtk", &create<CairoGtkDevice> },
		{ "software", &create<CairoSwDevice> },
		{ "sw", &create<CairoSwDevice> },
	};

	Device *Device::create(Engine &e) {
		const char *preference = getenv(ENV_RENDER_BACKEND);
		CreateDevice *selected = null;
		if (!preference) {
			selected = &devices[0];
		} else {
			for (size_t i = 0; i < ARRAY_COUNT(devices); i++) {
				if (strcmp(preference, devices[i].name) == 0) {
					selected = &devices[i];
					break;
				}
			}

			if (!selected) {
				StrBuf *msg = new (e) StrBuf();
				*msg << S("The supplied value of ") S(ENV_RENDER_BACKEND) S(" is not supported.\n");
				*msg << S("This build provides the following options: ");
				for (size_t i = 0; i < ARRAY_COUNT(devices); i++) {
					if (i > 0)
						*msg << S(", ");
					*msg << toWChar(e, devices[i].name)->v;
				}

				throw new (e) GuiError(msg->toS());
			}
		}

		Device *device = (*selected->create)(e);
		return applyEnvWorkarounds(device);
	}

#else
#error "Unknown UI toolkit."
#endif

#ifdef GUI_GTK

	static void reportError(Engine &e, GError *error) {
		StrBuf *msg = new (e) StrBuf();
		*msg << S("Initialization of OpenGL failed: ") << toWChar(e, error->message)->v << S("\n");
		*msg << S("Try setting the environment variable ") << S(ENV_RENDER_BACKEND) << S(" to \"gtk\" or \"software\".");
		g_clear_error(&error);

		throw new (e) GuiError(msg->toS());
	}

	GLDevice::GLDevice(Engine &e) : Device(), e(e) {}

	GLDevice::~GLDevice() {
		for (Map::iterator i = context.begin(); i != context.end(); ++i) {
			i->second->owner = null;
		}
	}

	Surface *GLDevice::createSurface(Handle window) {
		GtkWidget *widget = drawWidget(e, window);
		GdkWindow *w = gtk_widget_get_window(widget);
		if (!w)
			return null;

		Context *c = null;

		// Note: We don't own references in the map.
		Map::iterator found = context.find(w);
		if (found == context.end()) {
			c = createContext(w, createContext(w));

			// Figure out if we need any workarounds...
			gdk_gl_context_make_current(c->context);
			c->workarounds = glWorkarounds();

			context.insert(std::make_pair(w, c));
		} else {
			c = found->second;
			c->ref();
		}

		try {
			Surface *r = createSurface(widget, c);
			if (c->workarounds)
				r = c->workarounds->apply(r);
			return r;
		} catch (...) {
			c->unref();
			throw;
		}
	}

	GdkGLContext *GLDevice::createContext(GdkWindow *window) {
		GError *error = NULL;
		GdkGLContext *context = gdk_window_create_gl_context(window, &error);
		if (error) {
			g_object_unref(context);
			reportError(e, error);
		}

		gdk_gl_context_set_use_es(context, true);
		gdk_gl_context_set_required_version(context, 2, 0);

		gdk_gl_context_realize(context, &error);
		if (error) {
			g_object_unref(context);
			reportError(e, error);
		}

		return context;
	}

	GLDevice::Context *GLDevice::createContext(GdkWindow *window, GdkGLContext *context) {
		return new Context(this, window, context);
	}

	GLDevice::Context::Context(GLDevice *owner, GdkWindow *window, GdkGLContext *context)
		: window(window), context(context), id(0), refs(1), owner(owner), workarounds(null) {}

	GLDevice::Context::~Context() {
		// Remove us from our owner's map fo contexts. The owner might have been destroyed before
		// us, so keep that in mind.
		if (owner) {
			owner->context.erase(window);

			// Free the ID as well.
			if (id)
				renderMgr(owner->e)->freeId(id);
		}

		gdk_gl_context_clear_current();
		g_object_unref(context);

		delete workarounds;
	}

#endif

}
