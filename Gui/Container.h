#pragma once
#include "Window.h"
#include "Core/Map.h"
#include "Core/Array.h"

namespace gui {

	class RadioGroup;

	/**
	 * Container interface.
	 *
	 * Classes may implement this interface to indicate that they are able to store child
	 * windows. This is the minimal interface required by the windowing system. It does not allow
	 * adding/removing child windows. That is the responsibility of child classes.
	 *
	 * Also keeps track of groupings inside the container.
	 *
	 * Note: It is currently not possible to extend this class directly from Storm.
	 */
	class ContainerBase : public Window {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ContainerBase();

		// Get a radio group with a specified ID. Creates it if it does not exist.
		RadioGroup *STORM_FN radioGroup(Nat id);

#ifdef GUI_GTK
		// Add a child widget to the layout here.
		virtual void addChild(GtkWidget *child, Rect pos);

		// Move a child widget.
		virtual void moveChild(GtkWidget *child, Rect pos);
#endif

	private:
		// Radio button groups. Null until used.
		Map<Nat, RadioGroup *> *radioGroups;

		// Padding due to 8-byte alignment from Window.
		Nat pad;
	};

	/**
	 * A container is a window that can contain child windows.
	 *
	 * Manages IDs of child windows and forwards messages from child windows to their corresponding
	 * classes here.
	 */
	class Container : public ContainerBase {
		STORM_CLASS;
	public:
		// Create a container.
		STORM_CTOR Container();

		// Add a child window. This may actually create it!
		void STORM_FN add(Window *child);

		// Remove a child window. Silently fails if the child does not already exist.
		void STORM_FN remove(Window *child);

		// Get all children.
		Array<Window *> *STORM_FN children() const;

		// Called when our parent is created.
		virtual void parentCreated(nat id);

		// Called when we've been destroyed.
		virtual void windowDestroyed();


#ifdef GUI_WIN32
		// Handle messages.
		virtual MsgResult onMessage(const Message &msg);

		// Forward WM_COMMAND messages to the right window.
		bool onCommand(const Message &msg);

		// Forward WM_NOTIFY.
		bool onNotify(const Message &msg);

		// Update DPI.
		virtual void updateDpi(Bool move);
#endif
#ifdef GUI_GTK
		// Add a child widget to the layout here.
		virtual void addChild(GtkWidget *child, Rect pos);

		// Move a child widget.
		virtual void moveChild(GtkWidget *child, Rect pos);
#endif
	protected:
		// Create.
		virtual bool create(ContainerBase *parent, nat id);

	private:
		// Currently used ids.
		typedef Map<Nat, Window *> IdMap;
		Map<Nat, Window *> *ids;

		// Map the other way as well.
		typedef Map<Window *, Nat> WinMap;
		Map<Window *, Nat> *windows;

		// Last used control id.
		Nat lastId;

		// Padding due to 8-byte alignment from Window.
		Nat pad;

		// Allocate a new id for the window.
		Nat allocate(Window *window);

		// Allocate a window with a suggested id. Fails if that id is already in use.
		// An id of 0 is special. That means you do not want any callbacks from that control.
		void allocate(Window *window, Nat id);
	};

}
