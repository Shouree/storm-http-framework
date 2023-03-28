#include "stdafx.h"
#include "ScrollWindow.h"
#include "Container.h"
#include "Exception.h"
#include "Win32Dpi.h"

namespace gui {

	ScrollWindow::ScrollWindow(Window *inside) : child(inside), hScroll(false), vScroll(true) {
		if (inside->parent())
			throw new (this) GuiError(S("Can not attach a child multiple times or to multiple parents."));
		child->attachParent(this);
	}

	ScrollWindow::ScrollWindow(Window *inside, Size minSize)
		: child(inside), minSz(minSize), hScroll(false), vScroll(true) {

		if (inside->parent())
			throw new (this) GuiError(S("Can not attach a child multiple times or to multiple parents."));
		child->attachParent(this);
	}

	void ScrollWindow::windowDestroyed() {
		ContainerBase::windowDestroyed();

		if (child->created())
			child->handle(invalid);
	}

	void ScrollWindow::content(Window *n) {
		// Destroy the old one.
		if (child && child->created()) {
			child->detachParent();
		}

		// And attach the new one.
		child = n;
		child->attachParent(this);

		if (created())
			child->parentCreated(1);
		resized(Size());
	}


#ifdef GUI_WIN32

	void ScrollWindow::parentCreated(nat id) {
		ContainerBase::parentCreated(id);

		// Create the child with id 1.
		child->parentCreated(1);

		// Update the UI.
		resized(Size());
	}

	void ScrollWindow::updateDpi(Bool move) {
		if (child)
			child->updateDpi(true);
		ContainerBase::updateDpi(move);
	}

	void ScrollWindow::minSize(Size sz) {
		minSz = sz;
	}

	Size ScrollWindow::minSize() {
		Size ch = child->minSize() + Size(0.5); // Round up for a bit of extra margins when dealing with text.

		// Compensate for the size of the actual scrollbars.
		if (hScroll)
			ch.h += dpiSystemMetrics(SM_CYHSCROLL, defaultDpi);
		if (vScroll)
			ch.w += dpiSystemMetrics(SM_CXVSCROLL, defaultDpi);

		if (hScroll) {
			ch.w = min(ch.w, minSz.w);
		} else {
			ch.w = max(ch.w, minSz.w);
		}

		if (vScroll) {
			ch.h = min(ch.h, minSz.h);
		} else {
			ch.h = max(ch.h, minSz.h);
		}

		return ch;
	}

	void ScrollWindow::resized(Size size) {
		// Windows does not count the scrollbars as a part of the client area.
		if (created()) {
			RECT rect;
			GetWindowRect(handle().hwnd(), &rect);
			Float scale = dpiScaleInv(currentDpi());
			size.w = Float(rect.right - rect.left) * scale;
			size.h = Float(rect.bottom - rect.top) * scale;
		}

		ContainerBase::resized(size);

		Size sz = child->minSize();
		if (!hScroll) {
			sz.w = size.w;
		}

		if (!vScroll) {
			sz.h = size.h;
		}

		setChildSize(sz);
		updateBars(sz);
	}

	bool ScrollWindow::create(ContainerBase *parent, nat id) {
		DWORD myFlags = 0;
		if (hScroll)
			myFlags |= WS_HSCROLL;
		if (vScroll)
			myFlags |= WS_VSCROLL;
		return createEx(NULL, childFlags | myFlags, WS_EX_CONTROLPARENT, parent->handle().hwnd(), id);
	}

	void ScrollWindow::horizontal(Bool v) {
		hScroll = v;

		if (created()) {
			ShowScrollBar(handle().hwnd(), SB_HORZ, hScroll ? TRUE : FALSE);
		}
	}

	void ScrollWindow::vertical(Bool v) {
		vScroll = v;

		if (created()) {
			ShowScrollBar(handle().hwnd(), SB_VERT, vScroll ? TRUE : FALSE);
		}
	}

	MsgResult ScrollWindow::onMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_HSCROLL:
			return msgResult(onScroll(msg.wParam, SB_HORZ));
		case WM_VSCROLL:
			return msgResult(onScroll(msg.wParam, SB_VERT));
		case WM_MOUSEWHEEL:
			return msgResult(onWheel(GET_WHEEL_DELTA_WPARAM(msg.wParam), SB_VERT));
		case WM_MOUSEHWHEEL:
			return msgResult(onWheel(GET_WHEEL_DELTA_WPARAM(msg.wParam), SB_HORZ));
		}

		return ContainerBase::onMessage(msg);
	}

	LRESULT ScrollWindow::onScroll(WPARAM param, int which) {
		SCROLLINFO info;
		info.cbSize = sizeof(info);
		info.fMask = SIF_PAGE | SIF_POS | SIF_TRACKPOS | SIF_RANGE;
		GetScrollInfo(handle().hwnd(), which, &info);

		Float scale = dpiScale(currentDpi());
		const int lineSize = int(font()->pxHeight());

		switch (LOWORD(param)) {
		case SB_TOP:
			info.nPos = info.nMin;
			break;
		case SB_BOTTOM:
			info.nPos = info.nMax;
			break;
		case SB_LINEUP:
			info.nPos = max(info.nPos - lineSize, info.nMin);
			break;
		case SB_LINEDOWN:
			info.nPos = min(info.nPos + lineSize, info.nMax - int(info.nPage));
			break;
		case SB_PAGEDOWN:
			info.nPos = max(info.nPos - int(info.nPage), info.nMin);
			break;
		case SB_PAGEUP:
			info.nPos = min(info.nPos + int(info.nPage), info.nMax - int(info.nPage));
			break;
		case SB_THUMBTRACK:
			info.nPos = info.nTrackPos;
			break;
		default:
			return 0;
		}

		info.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
		SetScrollInfo(handle().hwnd(), which, &info, TRUE);

		int x = info.nPos, y = info.nPos;
		if (which == SB_HORZ) {
			SCROLLINFO info;
			info.cbSize = sizeof(info);
			info.fMask = SIF_POS;
			GetScrollInfo(handle().hwnd(), SB_VERT, &info);
			y = info.nPos;
		} else {
			SCROLLINFO info;
			info.cbSize = sizeof(info);
			info.fMask = SIF_POS;
			GetScrollInfo(handle().hwnd(), SB_HORZ, &info);
			x = info.nPos;
		}

		x = int(x * scale);
		y = int(y * scale);
		SetWindowPos(child->handle().hwnd(), NULL, -x, -y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);

		return 0;
	}

	LRESULT ScrollWindow::onWheel(int delta, int which) {
		SCROLLINFO info;
		info.cbSize = sizeof(info);
		info.fMask = SIF_PAGE | SIF_POS | SIF_TRACKPOS | SIF_RANGE;
		GetScrollInfo(handle().hwnd(), which, &info);

		Float scale = dpiScale(currentDpi());

		info.nPos = info.nPos - delta / 2;
		if (info.nPos < info.nMin)
			info.nPos = info.nMin;
		if (info.nPos > info.nMax - int(info.nPage))
			info.nPos = info.nMax - int(info.nPage);

		info.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
		SetScrollInfo(handle().hwnd(), which, &info, TRUE);

		int x = info.nPos, y = info.nPos;
		if (which == SB_HORZ) {
			SCROLLINFO info;
			info.cbSize = sizeof(info);
			info.fMask = SIF_POS;
			GetScrollInfo(handle().hwnd(), SB_VERT, &info);
			y = info.nPos;
		} else {
			SCROLLINFO info;
			info.cbSize = sizeof(info);
			info.fMask = SIF_POS;
			GetScrollInfo(handle().hwnd(), SB_HORZ, &info);
			x = info.nPos;
		}

		x = int(x * scale);
		y = int(y * scale);
		SetWindowPos(child->handle().hwnd(), NULL, -x, -y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);

		return 0;
	}

	void ScrollWindow::setScrollInfo(int which, Float childSz, Float ourSz) {
		SCROLLINFO info;
		info.cbSize = sizeof(info);
		info.fMask = SIF_POS;
		GetScrollInfo(handle().hwnd(), which, &info);

		info.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		info.nMin = 0;
		info.nMax = int(childSz);
		info.nPage = int(ourSz);
		info.nPos = min(info.nPos, info.nMax - int(info.nPage));
		SetScrollInfo(handle().hwnd(), which, &info, TRUE);
	}

	void ScrollWindow::updateBars(Size sz) {
		Nat dpi = currentDpi();
		Float scale = dpiScale(dpi);

		RECT ourSize;
		GetClientRect(handle().hwnd(), &ourSize);

		if (hScroll)
			setScrollInfo(SB_HORZ, sz.w, Float(ourSize.right) / scale);
		if (vScroll)
			setScrollInfo(SB_VERT, sz.h, Float(ourSize.bottom) / scale);

		// Update the position of the child, since we might need less scrolling.
		SCROLLINFO info;
		info.cbSize = sizeof(info);
		info.fMask = SIF_POS;
		GetScrollInfo(handle().hwnd(), SB_VERT, &info);
		int y = int(info.nPos * scale);
		GetScrollInfo(handle().hwnd(), SB_HORZ, &info);
		int x = int(info.nPos * scale);
		SetWindowPos(child->handle().hwnd(), NULL, -x, -y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	}

	void ScrollWindow::setChildSize(Size sz) {
		Nat dpi = currentDpi();
		sz = dpiToPx(dpi, sz);
		if (hScroll)
			sz.h -= dpiSystemMetrics(SM_CYHSCROLL, dpi);
		if (vScroll)
			sz.w -= dpiSystemMetrics(SM_CXVSCROLL, dpi);

		if (child->created()) {
			SetWindowPos(child->handle().hwnd(), NULL, 0, 0, (int)sz.w, (int)sz.h, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
		} else {
			child->pos(Rect(Point(), sz));
		}
	}

#endif
#ifdef GUI_GTK

	void ScrollWindow::minSize(Size sz) {
		minSz = sz;

		if (created()) {
			GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(handle().widget());
			gtk_scrolled_window_set_min_content_height(scroll, (gint)sz.h);
			gtk_scrolled_window_set_min_content_width(scroll, (gint)sz.w);
		}
	}

	Size ScrollWindow::minSize() {
		gint w = 0, h = 0;

		if (created()) {
			gtk_widget_get_preferred_width(handle().widget(), &w, NULL);
			gtk_widget_get_preferred_height(handle().widget(), &h, NULL);
		}

		return Size(max(minSz.w, Float(w)), max(minSz.h, Float(h)));
	}

	void ScrollWindow::resized(Size sz) {}

	bool ScrollWindow::create(ContainerBase *parent, nat id) {
		GtkWidget *widget = gtk_scrolled_window_new(NULL, NULL);

		GtkPolicyType hPolicy = hScroll ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER;
		GtkPolicyType vPolicy = vScroll ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER;
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget), hPolicy, vPolicy);

		minSize(minSz);

		initWidget(parent, widget);

		return true;
	}

	void ScrollWindow::horizontal(Bool v) {
		hScroll = v;

		if (created()) {
			GtkPolicyType hPolicy = hScroll ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER;
			GtkPolicyType vPolicy = vScroll ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER;
			gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(handle().widget()), hPolicy, vPolicy);
		}
	}

	void ScrollWindow::vertical(Bool v) {
		vScroll = v;

		if (created()) {
			GtkPolicyType hPolicy = hScroll ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER;
			GtkPolicyType vPolicy = vScroll ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER;
			gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(handle().widget()), hPolicy, vPolicy);
		}
	}

	void ScrollWindow::parentCreated(nat id) {
		Window::parentCreated(id);

		// Create the child with id 1.
		child->parentCreated(1);

		// Child widgets need to be realized for 'minSize' to work reasonably well.
		Size sz = this->child->minSize();
		gtk_widget_set_size_request(child->handle().widget(), (gint)sz.w, (gint)sz.h);
	}

	void ScrollWindow::addChild(GtkWidget *child, Rect pos) {
		// Remove any child already inside the scroll window (a Scrollable is added automatically)
		GtkWidget *content = gtk_bin_get_child(GTK_BIN(handle().widget()));
		if (content)
			gtk_container_remove(GTK_CONTAINER(handle().widget()), content);

		// Now we can add the new child.
		gtk_container_add(GTK_CONTAINER(handle().widget()), child);
	}

	void ScrollWindow::moveChild(GtkWidget *child, Rect pos) {}

	void ScrollWindow::setChildSize(Size sz) {}

#endif

}
