#include "stdafx.h"
#include "MnemonicStr.h"

namespace gui {

	MnemonicStr::MnemonicStr(Str *value) : value(value), hasPrefixes(false) {}

	void MnemonicStr::deepCopy(CloneEnv *) {
		// Nothing to do, strings are immutable.
	}

	Str *MnemonicStr::plain() const {
		if (!hasPrefixes)
			return value;

		StrBuf *out = new (value) StrBuf();
		bool lastUnderscore = false;
		for (Str::Iter i = value->begin(); i != value->end(); ++i) {
			bool underscore = i.v() == Char('_');

			if (underscore && lastUnderscore) {
				underscore = false;
			}

			if (!underscore) {
				*out << i.v();
			}

			lastUnderscore = underscore;
		}

		return out->toS();
	}

	Str *MnemonicStr::mnemonic(Char ch) const {
		if (hasPrefixes && ch == Char('_'))
			return value;

		StrBuf *out = new (value) StrBuf();
		if (hasPrefixes) {
			// Need to convert...
			bool lastUnderscore = false;
			for (Str::Iter i = value->begin(); i != value->end(); ++i) {
				bool underscore = i.v() == Char('_');

				if (underscore && lastUnderscore) {
					// No longer need to be escaped.
					underscore = false;
					lastUnderscore = false;
				}

				if (!underscore) {
					if (lastUnderscore) {
						// We want this to be underlined.
						*out << ch << i.v();
					} else if (i.v() == ch) {
						// Need to escape this character...
						*out << ch << ch;
					} else {
						*out << i.v();
					}
				}

				lastUnderscore = underscore;
			}
		} else {
			// Just need to escape 'ch' by duplicating it.
			for (Str::Iter i = value->begin(); i != value->end(); ++i) {
				if (i.v() == ch)
					*out << ch;
				*out << i.v();
			}
		}
		return out->toS();
	}

	void MnemonicStr::toS(StrBuf *to) const {
		if (hasPrefixes)
			*to << S("Mnemonic: ") << value;
		else
			*to << value;
	}

	MnemonicStr mnemonic(Str *value) {
		MnemonicStr x(value);
		x.hasPrefixes = true;
		return x;
	}

}
