#pragma once
#include "Core/StrBuf.h"

namespace gui {

	/**
	 * This value represents a string that may contain a prefix character to underline a letter used
	 * as a mnemonic and used as a hortcut.
	 *
	 * These prefix characters are used to highlight a character by drawing an underline under
	 * it. This is used to implement accessors (or mnemonics in Gtk) to access menu items using the
	 * keyboard etc.
	 *
	 * The default constructor of this class will treat the string as a regular string, and ensure
	 * that all characters in the string are escaped properly.
	 *
	 * When created with the "shortcut" function, then the character "_" is used to indicate the
	 * underlined character. Two underscores can be used to escape a single underscore.
	 *
	 * Note: It is not possible to have & and _ as mnemonics here, due to how these formats work (&
	 * is used internally on Win32).
	 */
	class MnemonicStr {
		STORM_VALUE;
	public:
		// Cast constructor, will interpret the string as a literal, even if underscores are present.
		STORM_CAST_CTOR MnemonicStr(Str *value);

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Get a string with all shortcut indicator removed.
		Str *STORM_FN plain() const;

		// Get a string that uses the specified mnemonic. Does proper escaping as needed.
		Str *STORM_FN mnemonic(Char ch) const;

		// Get a string that uses underscores to indicate mnemonics (the default).
		Str *STORM_FN mnemonic() const { return mnemonic(Char('_')); }

		// For Win32: convert underscores to ampersands as needed.
		Str *win32Mnemonic() const { return mnemonic(Char('&')); }

		// Output.
		void STORM_FN toS(StrBuf *to) const;

	private:
		// The original string.
		Str *value;

		// Shall we interpret prefixes in the string?
		Bool hasPrefixes;

		friend MnemonicStr STORM_FN mnemonic(Str *value);
	};

	// Create a MnemonicStr instance, indicating that we do want underscores to be interpreted as a mnemonic.
	MnemonicStr STORM_FN mnemonic(Str *value);

}
