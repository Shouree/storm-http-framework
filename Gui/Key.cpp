#include "stdafx.h"
#include "Key.h"

namespace gui {

	Nat keycode(Key key) {
		// Control keys do not have any characters assigned to them, except for return and tab.
		if (key >= key::firstControl && key < key::lastControl) {
			switch (key) {
			case key::ret:
				return '\n';
			case key::tab:
				return '\t';
			default:
				return 0;
			}
		}

		// Otherwise, the keycode is a codepoint.
		return Nat(key);
	}

	Char keychar(Key key) {
		return Char(keycode(key));
	}

#define KEY_NAME(enum, str)						\
	case key::enum:								\
	return S(str)

	const wchar *c_name(Key key) {
		switch (key) {
			KEY_NAME(shift, "Shift");
			KEY_NAME(control, "Ctrl");
			KEY_NAME(alt, "Menu");
			KEY_NAME(super, "Win");
			KEY_NAME(esc, "Esc");
			KEY_NAME(backspace, "Backspace");
			KEY_NAME(tab, "Tab");
			KEY_NAME(ret, "Return");
			KEY_NAME(left, "Left");
			KEY_NAME(right, "Right");
			KEY_NAME(up, "Up");
			KEY_NAME(down, "Down");
			KEY_NAME(pageUp, "Page Up");
			KEY_NAME(pageDown, "Page Down");
			KEY_NAME(home, "Home");
			KEY_NAME(end, "End");
			KEY_NAME(insert, "Insert");
			KEY_NAME(del, "Delete");
			KEY_NAME(F1, "F1");
			KEY_NAME(F2, "F2");
			KEY_NAME(F3, "F3");
			KEY_NAME(F4, "F4");
			KEY_NAME(F5, "F5");
			KEY_NAME(F6, "F6");
			KEY_NAME(F7, "F7");
			KEY_NAME(F8, "F8");
			KEY_NAME(F9, "F9");
			KEY_NAME(F10, "F10");
			KEY_NAME(F11, "F11");
			KEY_NAME(F12, "F12");
			KEY_NAME(F13, "F13");
			KEY_NAME(F14, "F14");
			KEY_NAME(F15, "F15");
			KEY_NAME(numpad0, "Num 0");
			KEY_NAME(numpad1, "Num 1");
			KEY_NAME(numpad2, "Num 2");
			KEY_NAME(numpad3, "Num 3");
			KEY_NAME(numpad4, "Num 4");
			KEY_NAME(numpad5, "Num 5");
			KEY_NAME(numpad6, "Num 6");
			KEY_NAME(numpad7, "Num 7");
			KEY_NAME(numpad8, "Num 8");
			KEY_NAME(numpad9, "Num 9");
			KEY_NAME(space, "<space>");
			KEY_NAME(num0, "0");
			KEY_NAME(num1, "1");
			KEY_NAME(num2, "2");
			KEY_NAME(num3, "3");
			KEY_NAME(num4, "4");
			KEY_NAME(num5, "5");
			KEY_NAME(num6, "6");
			KEY_NAME(num7, "7");
			KEY_NAME(num8, "8");
			KEY_NAME(num9, "9");
			KEY_NAME(a, "A");
			KEY_NAME(b, "B");
			KEY_NAME(c, "C");
			KEY_NAME(d, "D");
			KEY_NAME(e, "E");
			KEY_NAME(f, "F");
			KEY_NAME(g, "G");
			KEY_NAME(h, "H");
			KEY_NAME(i, "I");
			KEY_NAME(j, "J");
			KEY_NAME(k, "K");
			KEY_NAME(l, "L");
			KEY_NAME(m, "M");
			KEY_NAME(n, "N");
			KEY_NAME(o, "O");
			KEY_NAME(p, "P");
			KEY_NAME(q, "Q");
			KEY_NAME(r, "R");
			KEY_NAME(s, "S");
			KEY_NAME(t, "T");
			KEY_NAME(u, "U");
			KEY_NAME(v, "V");
			KEY_NAME(w, "W");
			KEY_NAME(x, "X");
			KEY_NAME(y, "Y");
			KEY_NAME(z, "Z");
			KEY_NAME(unknown, "<none>");
		default:
			return S("?");
		}
	}

	Str *name(EnginePtr e, Key k) {
		return new (e.v) Str(c_name(k));
	}

	static void put(StrBuf *to, bool &first, const wchar *str) {
		if (!first)
			*to << S("+");
		first = false;
		*to << str;
	}

	Str *name(EnginePtr e, Modifiers m) {
		StrBuf *out = new (e.v) StrBuf();
		bool first = true;
		if (m & mod::ctrl)
			put(out, first, S("Ctrl"));
		if (m & mod::alt)
			put(out, first, S("Alt"));
		if (m & mod::shift)
			put(out, first, S("Shift"));
		if (m & mod::super)
			put(out, first, S("Win"));
		return out->toS();
	}

	/**
	 * Map keycodes from backend specific keycodes.
	 */

	struct KeyEntry {
		Key storm;
		Nat system;
	};

#ifdef GUI_WIN32
#define KEYMAP(storm, win, gtk) { key::storm, win }
#endif
#ifdef GUI_GTK
#define KEYMAP(storm, win, gtk) { key::storm, gtk }
#endif

	static const KeyEntry keymap[] = {
		KEYMAP(shift, VK_SHIFT, GDK_KEY_Shift_L),
		KEYMAP(control, VK_CONTROL, GDK_KEY_Control_L),
		KEYMAP(alt, VK_MENU, GDK_KEY_Alt_L),
		KEYMAP(super, VK_LWIN, GDK_KEY_Super_L),
		KEYMAP(shift, VK_SHIFT, GDK_KEY_Shift_R),
		KEYMAP(control, VK_CONTROL, GDK_KEY_Control_R),
		KEYMAP(alt, VK_MENU, GDK_KEY_Alt_R),
		KEYMAP(super, VK_RWIN, GDK_KEY_Super_R),
		KEYMAP(menu, VK_APPS, GDK_KEY_Menu),
		KEYMAP(esc, VK_ESCAPE, GDK_KEY_Escape),
		KEYMAP(backspace, VK_BACK, GDK_KEY_BackSpace),
		KEYMAP(tab, VK_TAB, GDK_KEY_Tab),
		KEYMAP(ret, VK_RETURN, GDK_KEY_Return),
		KEYMAP(left, VK_LEFT, GDK_KEY_Left),
		KEYMAP(right, VK_RIGHT, GDK_KEY_Right),
		KEYMAP(up, VK_UP, GDK_KEY_Up),
		KEYMAP(down, VK_DOWN, GDK_KEY_Down),
		KEYMAP(pageUp, VK_PRIOR, GDK_KEY_Page_Up),
		KEYMAP(pageDown, VK_NEXT, GDK_KEY_Page_Down),
		KEYMAP(home, VK_HOME, GDK_KEY_Home),
		KEYMAP(end, VK_END, GDK_KEY_End),
		KEYMAP(insert, VK_INSERT, GDK_KEY_Insert),
		KEYMAP(del, VK_DELETE, GDK_KEY_Delete),
		KEYMAP(F1, VK_F1, GDK_KEY_F1),
		KEYMAP(F2, VK_F2, GDK_KEY_F2),
		KEYMAP(F3, VK_F3, GDK_KEY_F3),
		KEYMAP(F4, VK_F4, GDK_KEY_F4),
		KEYMAP(F5, VK_F5, GDK_KEY_F5),
		KEYMAP(F6, VK_F6, GDK_KEY_F6),
		KEYMAP(F7, VK_F7, GDK_KEY_F7),
		KEYMAP(F8, VK_F8, GDK_KEY_F8),
		KEYMAP(F9, VK_F9, GDK_KEY_F9),
		KEYMAP(F10, VK_F10, GDK_KEY_F10),
		KEYMAP(F11, VK_F11, GDK_KEY_F11),
		KEYMAP(F12, VK_F12, GDK_KEY_F12),
		KEYMAP(F13, VK_F13, GDK_KEY_F13),
		KEYMAP(F14, VK_F14, GDK_KEY_F14),
		KEYMAP(F15, VK_F15, GDK_KEY_F15),
		KEYMAP(numpad0, VK_NUMPAD0, GDK_KEY_KP_0),
		KEYMAP(numpad1, VK_NUMPAD1, GDK_KEY_KP_1),
		KEYMAP(numpad2, VK_NUMPAD2, GDK_KEY_KP_2),
		KEYMAP(numpad3, VK_NUMPAD3, GDK_KEY_KP_3),
		KEYMAP(numpad4, VK_NUMPAD4, GDK_KEY_KP_4),
		KEYMAP(numpad5, VK_NUMPAD5, GDK_KEY_KP_5),
		KEYMAP(numpad6, VK_NUMPAD6, GDK_KEY_KP_6),
		KEYMAP(numpad7, VK_NUMPAD7, GDK_KEY_KP_7),
		KEYMAP(numpad8, VK_NUMPAD8, GDK_KEY_KP_8),
		KEYMAP(numpad9,	VK_NUMPAD9, GDK_KEY_KP_9),
	};

	// Lookup a keycode. Returns 'unknown' if the key is not in the table above.
	static Key lookup(Nat sys) {
		// Make sure we do not have too many control characters.
		assert(key::lastControl < 0xDC00);

		// Note: A linear search is probably fast enough here. There are not that many cases.
		for (Nat i = 0; i < ARRAY_COUNT(keymap); i++) {
			if (keymap[i].system == sys)
				return keymap[i].storm;
		}

		return key::unknown;
	}

	// Reverse of the above lookup.
	static Nat reverseLookup(Key key) {
		for (Nat i = 0; i < ARRAY_COUNT(keymap); i++) {
			if (keymap[i].storm == key)
				return keymap[i].system;
		}

		return 0;
	}

#ifdef GUI_WIN32

	bool pressed(nat keycode) {
		return (GetKeyState(keycode) & 0x8000) != 0;
	}

	Modifiers modifiers() {
		Modifiers r = mod::none;
		if (pressed(VK_CONTROL))
			r |= mod::ctrl;
		if (pressed(VK_MENU))
			r |= mod::alt;
		if (pressed(VK_SHIFT))
			r |= mod::shift;
		// Super?
		return r;
	}

	Key keycode(WPARAM vk) {
		Key result = lookup(Nat(vk));
		if (result == key::unknown) {
			UINT c = MapVirtualKey(Nat(vk), MAPVK_VK_TO_CHAR);
			c &= 0x7FFFFFFF; // The highest bit is used to indicate dead keys. We do not care at this point.

			// Windows gives uppercase for A-Z, but lowercase to any other characters that have uppercase forms...
			if (c >= 'A' && c <= 'Z')
				c += 'a' - 'A';

			result = Key(c);
		}
		return result;
	}

#endif
#ifdef GUI_GTK

	Modifiers modifiers(const GdkEventKey &event) {
		guint state = event.state;
		Modifiers r = mod::none;
		if (state & GDK_CONTROL_MASK)
			r |= mod::ctrl;
		if (state & GDK_META_MASK)
			r |= mod::alt;
		if (state & GDK_SHIFT_MASK)
			r |= mod::shift;
		if (state & GDK_SUPER_MASK)
			r |= mod::super;
		return r;
	}

	Key keycode(const GdkEventKey &event) {
		guint hw = event.hardware_keycode;
		GdkKeymap *keymap = gdk_keymap_get_for_display(gdk_window_get_display(event.window));

		guint keyval = 0;
		// Note: We're ignoring 'state' and 'group' to get plain keypresses, just like in Win32.
		gdk_keymap_translate_keyboard_state(keymap, hw, (GdkModifierType)0, 0, &keyval, NULL, NULL, NULL);

		Key result = lookup(keyval);
		if (result == key::unknown) {
			// Let's hope it is a valid character!
			result = Key(keyval);
		}
		return result;
	}

	Key from_gtk(guint key) {
		Key result = lookup(key);
		if (result == key::unknown)
			result = Key(key);
		return result;
	}

	Modifiers from_gtk(GdkModifierType mod) {
		guint state = mod;
		Modifiers r = mod::none;
		if (state & GDK_CONTROL_MASK)
			r |= mod::ctrl;
		if (state & GDK_META_MASK)
			r |= mod::alt;
		if (state & GDK_SHIFT_MASK)
			r |= mod::shift;
		if (state & GDK_SUPER_MASK)
			r |= mod::super;
		return r;
	}

	guint to_gtk(Key key) {
		Nat v = reverseLookup(key);
		if (v == 0) {
			// Assume it is a keycode, so that we can invert what "keycode" gives us.
			return guint(key);
		} else {
			return v;
		}
	}

	GdkModifierType to_gtk(Modifiers mod) {
		Nat result = 0;
		if (mod & mod::ctrl)
			result |= GDK_CONTROL_MASK;
		if (mod & mod::alt)
			result |= GDK_META_MASK;
		if (mod & mod::shift)
			result |= GDK_SHIFT_MASK;
		if (mod & mod::super)
			result |= GDK_SUPER_MASK;
		return (GdkModifierType)result;
	}

#endif

}
