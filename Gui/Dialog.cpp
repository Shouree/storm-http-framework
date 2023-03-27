#include "stdafx.h"
#include "Dialog.h"
#include "Exception.h"
#include "GtkSignal.h"

namespace gui {

	Dialog::Dialog(Str *title) : Frame(title), parent(null) {}

	Dialog::Dialog(Str *title, Size size) : Frame(title, size), parent(null) {}

	void Dialog::close() {
		close(-1);
	}

	void Dialog::defaultChoice(Button *button) {
		if (defaultButton)
			defaultButton->setDefault(false);
		button->setDefault(true);
		button->onClick = fnPtr(engine(), &Dialog::onOk, this);
		defaultButton = button;
	}

	void Dialog::onOk() {
		close(1);
	}

	void Dialog::onDestroy(Int code) {}

#ifdef GUI_WIN32

	Int Dialog::show(Frame *parent) {
		if (handle() != invalid)
			throw new (this) GuiError(S("Don't call 'create' before calling 'show' on a dialog."));

		this->parent = parent;
		createWindow(true, parent);

		// Make us modal!
		EnableWindow(parent->handle().hwnd(), FALSE);
		waitForClose();
		this->parent = null;
		return result;
	}

	void Dialog::close(Int result) {
		onDestroy(result);
		this->result = result;
		// Make sure to enable the old window before we remove ourselves.
		if (parent) {
			EnableWindow(parent->handle().hwnd(), TRUE);
			SetFocus(parent->handle().hwnd());
		}
		Frame::close();
	}

	MsgResult Dialog::onMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_COMMAND: {
			nat type = HIWORD(msg.wParam);
			nat id = LOWORD(msg.wParam);

			if (type == 0) {
				// Accelerator. Check for IDOK or IDCANCEL.
				if (id == IDOK) {
					if (defaultButton)
						defaultButton->onCommand(BN_CLICKED);
					return msgResult(TRUE);
				} else if (id == IDCANCEL) {
					close();
					return msgResult(TRUE);
				}
			}
		}
		}

		return Frame::onMessage(msg);
	}

#endif

#ifdef GUI_GTK

	Int Dialog::show(Frame *parent) {
		if (handle() != invalid)
			throw new (this) GuiError(S("Don't call 'create' before calling 'show' on a dialog."));

		this->parent = parent;
		createWindow(true, parent);

		if (defaultButton) {
			defaultButton->setDefault(true);
		}

		gint result = gtk_dialog_run(GTK_DIALOG(handle().widget()));

		// This is the default "close" action.
		if (result == -4)
			result = -1;

		// Close the window.
		Frame::close();

		this->parent = null;
		return result;
	}

	void Dialog::initSignals(GtkWidget *widget, GtkWidget *draw) {
		Frame::initSignals(widget, draw);
		Signal<gboolean, Dialog, GdkEvent *>::Connect<&Dialog::onKey>::to(widget, "key-press-event", engine());
	}

	gboolean Dialog::onKey(GdkEvent *event) {
		GdkEventKey &k = event->key;
		Nat key = keycode(k);
		Modifiers mod = modifiers(k);

		if (key == key::ret && mod == mod::none) {
			if (defaultButton) {
				defaultButton->clicked();
				return TRUE;
			}
		}

		return FALSE;
	}

	void Dialog::close(Int result) {
		onDestroy(result);

		// If opened as a regular window, it might not be a dialog.
		if (GTK_IS_DIALOG(handle().widget())) {
			gtk_dialog_response(GTK_DIALOG(handle().widget()), result);
		} else {
			Frame::close();
		}
	}

#endif

}
