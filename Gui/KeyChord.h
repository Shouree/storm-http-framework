#pragma once
#include "Key.h"
#include "Core/Hash.h"

namespace gui {

	/**
	 * A keyboard combination that can be used as a hotkey.
	 *
	 * Consists of a key and a set of modifiers.
	 */
	class KeyChord {
		STORM_VALUE;
	public:
		// Create an "empty" chord.
		STORM_CTOR KeyChord() : key(gui::key::unknown), modifiers(mod::none) {}

		// Create an accelerator with only a key.
		STORM_CTOR KeyChord(key::Key key) : key(key), modifiers(mod::none) {}

		// Create an accelerator with a modifier and a key.
		STORM_CTOR KeyChord(key::Key key, mod::Modifiers modifiers) : key(key), modifiers(modifiers) {}

		// Key.
		key::Key key;

		// Modifiers.
		mod::Modifiers modifiers;

		// Empty?
		Bool STORM_FN empty() const {
			return key == gui::key::unknown;
		}
		Bool STORM_FN any() const {
			return key != gui::key::unknown;
		}

		// Compare for equality.
		Bool STORM_FN operator ==(KeyChord o) const {
			return key == o.key && modifiers == o.modifiers;
		}

		// Hash.
		Nat STORM_FN hash() const {
			return natHash((Nat(key) << 4) | Nat(modifiers));
		}

		// Output.
		void STORM_FN toS(StrBuf *to) const;
	};

	// Create a key chord with Ctrl.
	inline KeyChord STORM_FN ctrlChord(key::Key key) {
		return KeyChord(key, mod::ctrl);
	}

	// Create a key chord with Alt.
	inline KeyChord STORM_FN altChord(key::Key key) {
		return KeyChord(key, mod::alt);
	}

}
