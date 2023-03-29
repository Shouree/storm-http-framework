#include "stdafx.h"
#include "TabView.h"
#include "Win32Dpi.h"

namespace gui {

	TabView::TabView() {
		children = new (this) Array<Window *>();
		titles = new (this) Array<Str *>();
	}

	void TabView::windowDestroyed() {
		ContainerBase::windowDestroyed();

		for (Nat i = 0; i < children->count(); i++) {
			Window *child = children->at(i);
			if (child->created())
				child->handle(invalid);
		}
	}

	void TabView::add(Str *title, Window *child) {
		children->push(child);
		titles->push(title);

		child->attachParent(this);

		if (created()) {
			// We need to tell the child to create itself before we add the tab. Otherwise the Gtk
			// code will not work.
			child->parentCreated(children->count());

			tabAdded(child, title, children->count() - 1);
		}
	}

#ifdef GUI_WIN32

	void TabView::selected(Nat tab) {
		if (created()) {
			if (currentSelected < children->count())
				children->at(currentSelected)->visible(false);
			if (tab < children->count())
				children->at(tab)->visible(true);
		}

		currentSelected = tab;
	}

	Size TabView::minSize() {
		Size result;
		for (Nat i = 0; i < children->count(); i++)
			result = result.max(children->at(i)->minSize());

		if (created()) {
			// We can just use the Win32 API to adjust the size.
			Nat dpi = currentDpi();
			Size px = dpiToPx(dpi, result);
			RECT r = convert(Rect(Point(), px));
			TabCtrl_AdjustRect(handle().hwnd(), TRUE, &r);
			result = convert(r).size();
		} else {
			// We could try to do this manually, but many controls don't report an accurate size
			// until they are created, so this is fine.
		}
		return result;
	}

	void TabView::parentCreated(nat id) {
		ContainerBase::parentCreated(id);

		// Create children with ID from 1 and upwards.
		for (Nat i = 0; i < children->count(); i++)
			children->at(i)->parentCreated(i + 1);

		// Update UI?
		resized(pos().size());
	}

	void TabView::pos(Rect r) {
		ContainerBase::pos(r);

		// We need this since we won't get the messages sent to the control in general (at least not
		// the ones we send ourselves).
		resized(r.size());
	}

	void TabView::resized(Size size) {
		if (created()) {
			Nat dpi = currentDpi();
			dpiToPx(dpi, size);

			RECT position = {
				0, 0,
				LONG(size.w), LONG(size.h)
			};
			TabCtrl_AdjustRect(handle().hwnd(), FALSE, &position);

			for (Nat i = 0; i < children->count(); i++) {
				Window *child = children->at(i);
				if (child->created()) {
					// Call movewindow directly so we don't have to convert back and forward through DPI.
					MoveWindow(child->handle().hwnd(),
							position.left, position.top,
							position.right - position.left, position.bottom - position.top, TRUE);
				}
			}
		}
	}

	bool TabView::onNotify(NMHDR *header) {
		if (header->code == TCN_SELCHANGE) {
			selected(TabCtrl_GetCurSel(handle().hwnd()));
		}
		return true;
	}

	bool TabView::create(ContainerBase *parent, nat id) {
		DWORD flags = controlFlags | WS_CLIPSIBLINGS;
		if (!createEx(WC_TABCONTROL, flags, WS_EX_CONTROLPARENT, parent->handle().hwnd(), id))
			return false;

		for (Nat i = 0; i < children->count(); i++)
			tabAdded(children->at(i), titles->at(i), i);

		if (currentSelected < children->count())
			children->at(currentSelected)->visible(true);

		return true;
	}

	void TabView::updateDpi(Bool move) {
		for (Nat i = 0; i < children->count(); i++)
			children->at(i)->updateDpi(true);
		ContainerBase::updateDpi(move);
	}

	void TabView::tabAdded(Window *child, Str *title, Nat id) {
		child->visible(false);

		TCITEM item;
		item.mask = TCIF_TEXT | TCIF_PARAM;
		item.pszText = (LPWSTR)title->c_str();
		item.lParam = id;
		TabCtrl_InsertItem(handle().hwnd(), id, &item);
	}

#endif
#ifdef GUI_GTK

	Size TabView::minSize() {
		gint w = 0, h = 0;

		if (created()) {
			gtk_widget_get_preferred_width(handle().widget(), &w, NULL);
			gtk_widget_get_preferred_height(handle().widget(), &h, NULL);
		}

		return Size(Float(w), Float(h));
	}

	void TabView::parentCreated(nat id) {
		// Tell our children to create themselves first. This makes it possible for us to add them
		// directly later on.
		for (Nat i = 0; i < children->count(); i++)
			children->at(i)->parentCreated(i + 1);

		Window::parentCreated(id);
		// Handle size?
	}

	bool TabView::create(ContainerBase *parent, nat id) {
		GtkWidget *widget = gtk_notebook_new();

		initWidget(parent, widget);

		for (Nat i = 0; i < children->count(); i++)
			tabAdded(children->at(i), titles->at(i), i);

		return true;
	}

	void TabView::moveChild(GtkWidget *child, Rect pos) {
		// We don't honor requests from children.
	}

	void TabView::selected(Nat tab) {
		currentSelected = tab;
	}

	void TabView::addChild(GtkWidget *child, Rect pos) {
		// We don't do anything here.
	}

	void TabView::tabAdded(Window *child, Str *title, Nat id) {
		GtkNotebook *notebook = GTK_NOTEBOOK(handle().widget());
		gtk_notebook_append_page(notebook, child->handle().widget(), gtk_label_new(title->utf8_str()));
	}

	void TabView::pos(Rect r) {
		// We don't need to do anything special on Gtk.
		ContainerBase::pos(r);
	}

	void TabView::resized(Size size) {
		// We don't need to do anything special on Gtk.
		ContainerBase::resized(size);
	}

#endif

}
