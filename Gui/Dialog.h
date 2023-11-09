#pragma once
#include "Frame.h"
#include "Button.h"

namespace gui {

	/**
	 * A dialog is a Frame that is modal, i.e. it disallows interaction with some parent frame
	 * window until it is closed. The dialog also implements "show()", which blocks until the dialog
	 * is closed, and returns some value to the caller, which represents the result of the dialog.
	 *
	 * TODO: We want to provide some way of determining what happens when the user presses return
	 * and/or escape.
	 */
	class Dialog : public Frame {
		STORM_CLASS;
	public:
		// Create the dialog. As with `Frame`s, the dialog is not shown until `show` is called.
		STORM_CTOR Dialog(Str *title);
		STORM_CTOR Dialog(Str *title, Size size);

		// Show the dialog as a modal window. Don't call `create` before calling `show`, that is
		// handled internally. Returns whatever is passed to `close` by the implementation, or -1
		// if the dialog was closed (or cancelled by a default button).
		Int STORM_FN show(Frame *parent);

		// Call when the window is shown to close the dialog with a particular code.
		void STORM_FN close(Int result);

		// Override the default behavior to return -1.
		virtual void STORM_FN close();

		// Set the button that acts as the default choice in this dialog. This button will be
		// activated whenever "enter" is pressed. The button's handler will also be set by the
		// implementation to return 1, but this may be overridden by the user at a later point if
		// desired.
		void STORM_ASSIGN defaultChoice(Button *button);

		// Called when the dialog is about to be closed to give the implementation time to save
		// things before they are destroyed.
		virtual void STORM_FN onDestroy(Int code);

#ifdef GUI_WIN32
		// Handle the dialog messages.
		virtual MsgResult onMessage(const Message &msg);
#endif

	private:
		// Our parent. Only valid while we're being shown.
		MAYBE(Frame *) parent;

		// Remember the result.
		Int result;

		// Remember the control that is the default one.
		MAYBE(Button *) defaultButton;

		// Default OK handler.
		void CODECALL onOk();

#ifdef GUI_GTK
		// Gtk+ signals
		gboolean onKey(GdkEvent *event);
		virtual void initSignals(GtkWidget *widget, GtkWidget *draw);
#endif
	};

}
