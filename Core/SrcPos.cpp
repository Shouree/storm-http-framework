#include "stdafx.h"
#include "SrcPos.h"
#include "Core/CloneEnv.h"
#include "Core/StrBuf.h"

namespace storm {

	SrcPos::SrcPos() : file(null), start(0), end(0) {}

	SrcPos::SrcPos(Url *file, Nat start, Nat end) : file(file), start(min(start, end)), end(max(start, end)) {}

	Bool SrcPos::unknown() const {
		return file == null;
	}

	Bool SrcPos::any() const {
		return file != null;
	}

	Bool SrcPos::empty() const {
		return file == null;
	}

	SrcPos SrcPos::operator +(Nat v) const {
		return SrcPos(file, start + v, end + v);
	}

	SrcPos SrcPos::extend(SrcPos o) const {
		if (file != o.file) {
			if (unknown())
				return o;
			if (o.unknown())
				return *this;

			if (*file != *o.file)
				return *this;
		}

		// Now we know they are equal.
		return SrcPos(file, min(start, o.start), max(end, o.end));
	}

	SrcPos SrcPos::firstCh() const {
		return SrcPos(file, start, start + 1);
	}

	SrcPos SrcPos::lastCh() const {
		if (end > 0)
			return SrcPos(file, end - 1, end);
		else
			return SrcPos(file, end, end);
	}

	void SrcPos::deepCopy(CloneEnv *env) {
		cloned(file, env);
	}

	Bool SrcPos::operator ==(SrcPos o) const {
		if (!file && !o.file)
			return true;
		if (!file || !o.file)
			return false;
		return *file == *o.file
			&& start == o.start
			&& end == o.end;
	}

	Bool SrcPos::operator !=(SrcPos o) const {
		return !(*this == o);
	}

	wostream &operator <<(wostream &to, const SrcPos &p) {
		if (p.unknown())
			return to << L"<unknown location>";
		else
			return to << p.file << L"(" << p.start << L"-" << p.end << L")";
	}

	void SrcPos::toS(StrBuf *to) const {
		if (unknown())
			*to << S("<unknown location>");
		else
			*to << file << S("(") << start << S("-") << end << S(")");
	}

}
