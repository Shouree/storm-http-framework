#pragma once
#include "Container.h"

namespace gui {

	/**
	 * A window that presents the user with a number of "tabs" that each contain a separate
	 * sub-window.
	 *
	 * Automatically manages visibility of the sub-windows corresponding to the tabs. This effect
	 * may or may not be visible through the API depending on the windowing API used.
	 */
	class TabView : public ContainerBase {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR TabView();

		// Get minimum size.
		virtual Size STORM_FN minSize();

		// Add a tab.
		void STORM_FN add(Str *title, Window *child);

		// Called when our parent is created.
		virtual void parentCreated(nat id);

		// Called when we've been destroyed.
		virtual void windowDestroyed();

		// Called when resized.
		virtual void STORM_FN resized(Size size);

		// Get/set selected tab.
		void STORM_ASSIGN selected(Nat tab);
		Nat STORM_FN selected() const { return currentSelected; }

		// Hook into position updating code.
		virtual void STORM_ASSIGN pos(Rect r);
		using Window::pos;

#ifdef GUI_GTK
		// Add a child widget to the layout here.
		virtual void addChild(GtkWidget *child, Rect pos);

		// Move a child widget.
		virtual void moveChild(GtkWidget *child, Rect pos);
#endif
#ifdef GUI_WIN32
		// Handle notifications.
		virtual bool onNotify(NMHDR *header);

		// Update DPI.
		virtual void updateDpi(Bool move);
#endif

	protected:
		// Create.
		virtual bool create(ContainerBase *parent, nat id);

	private:
		// Child windows.
		Array<Window *> *children;

		// Title of the tabs.
		Array<Str *> *titles;

		// Current selected tab.
		Nat currentSelected;

		// Update control state.
		void tabAdded(Window *child, Str *title, Nat id);
	};

}
