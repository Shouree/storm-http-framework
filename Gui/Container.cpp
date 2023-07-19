#include "stdafx.h"
#include "Container.h"
#include "Exception.h"
#include "RadioButton.h"
#include <limits>

namespace gui {

	ContainerBase::ContainerBase() {}

	RadioGroup *ContainerBase::radioGroup(Nat id) {
		if (!radioGroups)
			radioGroups = new (this) Map<Nat, RadioGroup *>();

		Map<Nat, RadioGroup *>::Iter i = radioGroups->find(id);
		if (i != radioGroups->end())
			return i.v();

		RadioGroup *g = new (this) RadioGroup();
		radioGroups->put(id, g);
		return g;
	}

#ifdef GUI_GTK
	void ContainerBase::addChild(GtkWidget *child, Rect pos) {
		throw new (this) InternalError(S("Need to implement 'addChild' for custom containers!"));
	}

	void ContainerBase::moveChild(GtkWidget *child, Rect pos) {
		throw new (this) InternalError(S("Need to implement 'moveChild' for custom containers!"));
	}
#endif

#ifdef GUI_WIN32
	// Reserved ids.
	static const Nat reservedTo = IDCANCEL;
#else
	static const Nat reservedTo = 0;
#endif

	Container::Container() {
		ids = new (this) IdMap();
		windows = new (this) WinMap();
		lastId = reservedTo;
	}

	void Container::add(Window *child) {
		if (child->parent())
			throw new (this) GuiError(S("Can not attach a child multiple times or to multiple parents."));

		nat id = allocate(child);
		child->attachParent(this);

		if (created())
			child->parentCreated(id);
	}

	void Container::remove(Window *child) {
		if (!windows->has(child))
			return;

		child->detachParent();
		Nat id = windows->get(child);
		ids->remove(id);
		windows->remove(child);
	}

	Array<Window *> *Container::children() const {
		Array<Window *> *result = new (this) Array<Window *>();
		result->reserve(ids->count());
		for (IdMap::Iter i = ids->begin(), end = ids->end(); i != end; ++i) {
			result->push(i.v());
		}
		return result;
	}

	void Container::parentCreated(Nat id) {
		Window::parentCreated(id);

		// Late creation, create any remaining children!
		for (IdMap::Iter i = ids->begin(), end = ids->end(); i != end; ++i) {
			Window *w = i.v();
			if (!w->created())
				w->parentCreated(i.k());
		}
	}

	void Container::windowDestroyed() {
		Window::windowDestroyed();

		for (IdMap::Iter i = ids->begin(), end = ids->end(); i != end; ++i) {
			Window *w = i.v();
			if (w->created())
				w->handle(invalid);
		}
	}

	Nat Container::allocate(Window *window) {
		Nat firstFree;
		// Fallback in rare cases with a lot of creation/destruction.
		if (lastId == std::numeric_limits<Nat>::max()) {
			WARNING(L"Wrapping. Tab order will not be as expected!");
			// Wrap around and find something not used... We will most likely not have 2^32 controls
			// in a window, so we should find an empty id somewhere!
			for (firstFree = reservedTo + 1; ids->has(firstFree); firstFree++)
				;
		} else {
			firstFree = ++lastId;
		}

		ids->put(firstFree, window);
		windows->put(window, firstFree);
		return firstFree;
	}

	void Container::allocate(Window *window, Nat id) {
		if (id == 0)
			return;

		if (ids->has(id))
			throw new (this) GuiError(TO_S(this, S("The id ") << id << S(" is already in use.")));

		ids->put(id, window);
		windows->put(window, id);
	}

#ifdef GUI_WIN32

	bool Container::create(ContainerBase *parent, nat id) {
		// Make sure it is a control parent to make tab and control focus work properly.
		return Window::createEx(NULL, childFlags, WS_EX_CONTROLPARENT, parent->handle().hwnd(), id);
	}

	MsgResult Container::onMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_COMMAND:
			if (onCommand(msg))
				return msgResult(0);
			break;
		case WM_NOTIFY:
			if (onNotify(msg))
				return msgResult(0);
			break;
		}

		return Window::onMessage(msg);
	}

	bool Container::onCommand(const Message &msg) {
		nat type = HIWORD(msg.wParam);
		nat id = LOWORD(msg.wParam);

		// Menus and accelerators. Not implemented yet.
		if (msg.lParam == 0 || msg.lParam == 1)
			return false;

		IdMap::Iter i = ids->find(id);
		if (i == ids->end())
			return false;

		return i.v()->onCommand(type);
	}

	bool Container::onNotify(const Message &msg) {
		NMHDR *data = (NMHDR *)msg.lParam;
		nat id = nat(data->idFrom);

		IdMap::Iter i = ids->find(id);
		if (i == ids->end())
			return false;

		return i.v()->onNotify(data);
	}

	void Container::updateDpi(Bool move) {
		Window::updateDpi(move);

		for (IdMap::Iter i = ids->begin(), end = ids->end(); i != end; ++i) {
			Window *w = i.v();
			w->updateDpi(true);
		}
	}

#endif
#ifdef GUI_GTK

	bool Container::create(ContainerBase *parent, nat id) {
		// We need an event box to catch events.
		GtkWidget *box = gtk_event_box_new();

		GtkWidget *container = basic_new();
		gtk_container_add(GTK_CONTAINER(box), container);
		gtk_widget_show(container);

		initWidget(parent, box);
		return true;
	}

	void Container::addChild(GtkWidget *child, Rect pos) {
		// There is a BASIC layout inside the frame.
		Basic *basic = BASIC(gtk_bin_get_child(GTK_BIN(handle().widget())));
		Size s = pos.size();
		basic_put(basic, child, pos.p0.x, pos.p0.y, s.w, s.h);
	}

	void Container::moveChild(GtkWidget *child, Rect pos) {
		// There is a BASIC layout inside the frame.
		Basic *basic = BASIC(gtk_bin_get_child(GTK_BIN(handle().widget())));
		Size s = pos.size();
		basic_move(basic, child, pos.p0.x, pos.p0.y, s.w, s.h);
	}

#endif

}
