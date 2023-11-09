#pragma once
#include "Container.h"
#include "Core/Event.h"
#include "Menu.h"
#include "Accelerators.h"

namespace gui {

	/**
	 * A frame is a window with a border, present on the desktop. Creating the Frame does not make it visible.
	 *
	 * The frame has a somewhat special life time management. The frame will keep itself alive until it is closed.
	 */
	class Frame : public Container {
		STORM_CLASS;
	public:
		// Create a frame, specifying a window title. Note: `create` must be called to create the frame.
		STORM_CTOR Frame(Str *title);

		// Create a frame, specifying a window title and an initial size. Note: `create` must be called to create the frame.
		STORM_CTOR Frame(Str *title, Size size);

		// Create the frame and show it.
		void STORM_FN create();

		// Close this frame.
		virtual void STORM_FN close();

		// Wait until this frame is closed.
		void STORM_FN waitForClose();

#ifdef GUI_WIN32
		// Message!
		virtual MsgResult onMessage(const Message &msg);

		// Message: for handling accelerators.
		virtual MsgResult beforeMessage(const Message &msg);

		// Current DPI.
		virtual Nat currentDpi();
#endif

#ifdef GUI_GTK
		// Window text.
		using Window::text;
		virtual void STORM_ASSIGN text(Str *str);

		// Add a child widget to the layout here.
		virtual void addChild(GtkWidget *child, Rect pos);

		// Move a child widget.
		virtual void moveChild(GtkWidget *child, Rect pos);

		// Get the container widget inside here.
		virtual GtkWidget *drawWidget();
#endif

		// Set the size of the frame.
		virtual void STORM_ASSIGN size(Size s);

		// Set position.
		virtual void STORM_ASSIGN pos(Rect r);
		using Window::pos;

		// Get position.
		virtual Rect STORM_FN pos();

		// Set fullscreen mode.
		void STORM_ASSIGN fullscreen(Bool f);

		// Get fullscreen mode.
		Bool STORM_FN fullscreen();

		// Set if the cursor is visible when over this frame.
		void STORM_ASSIGN cursorVisible(Bool v);

		// Get if the cursor is visible when over this frame.
		Bool STORM_FN cursorVisible();

		// Get the accelerator table for this window.
		Accelerators *STORM_FN accelerators() const { return myAccelerators; }

		// Set the menu for this window.
		void STORM_ASSIGN menu(MAYBE(MenuBar *) menu);

		// Get the menu for this window.
		MAYBE(MenuBar *) STORM_FN menu();

		// Show a popup menu at the cursor position. This menu may be a part of another menu.
		void STORM_FN popupMenu(PopupMenu *menu);

		// Find a menu item from a handle, either in the attached window menu or from any popup
		// menus associated with this window.
		MAYBE(Menu::Item *) findMenuItem(Handle handle);

		// Remember to set focus to the particular window on creation.
		void focus(Window *child);

	protected:
		// Notification on window resizes.
		virtual void onResize(Size size);

		// Called when we're about to destroy the window.
		virtual void destroyWindow(Handle handle);

		// Helper to create the window. Optionally provide a parent window.
		bool createWindow(bool sizeable, MAYBE(Frame *) parent);

	private:
		// Update the minimum size.
		void updateMinSize();

		// Set the menu properly.
		void setMenu(MAYBE(MenuBar *) last);

		// Event that fires when we're closed.
		Event *onClose;

		// Window menu.
		MAYBE(MenuBar *) myMenu;

		// Accelerator table.
		Accelerators *myAccelerators;

		// Popup menu we're tracking.
		MAYBE(PopupMenu *) myPopup;

		// Window to assign focus after creation. Only used on Windows currently.
		MAYBE(Window *) setFocus;

		// Cached minimum size. Updated whenever the window is resized.
		Size lastMinSize;

		// Current DPI for this window. Only used on Win32 at the moment (GTK+ seems to do this internally).
		Nat dpi;

		// Info. Not valid if we're not in fullscreen mode.
		Nat windowedStyle;
		Rect windowedRect;

		// Fullscreen mode?
		Bool full;

		// Cursor visible?
		Bool showCursor;

		// Was our position altered?
		Bool posSet;

#ifdef GUI_WIN32
		// Find the clicked menu item.
		void menuClicked(HMENU menu, Nat id);
#endif
	};

}
