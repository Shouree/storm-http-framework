#include "stdafx.h"
#include "RefSource.h"
#include "Reference.h"
#include "Exception.h"
#include "Core/Array.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"

namespace code {

	RefSource::RefSource() : cont(null) {
		refs = new (this) WeakSet<Reference>();
	}

	RefSource::RefSource(Content *content) : cont(content) {
		refs = new (this) WeakSet<Reference>();
		if (cont)
			atomicWrite(cont->owner, this);
	}

	void RefSource::set(Content *to) {
		if (cont != to) {
			assert(to->owner == null, L"Multiple owners of a single Content object.");

			if (cont)
				atomicWrite(cont->owner, (RefSource *)null);
			if (to)
				atomicWrite(to->owner, this);
			cont = to;
		}

		update();
	}

	void RefSource::update() {
		const void *addr = address();
		WeakSet<Reference>::Iter i = refs->iter();
		while (Reference *ref = i.next())
			ref->moved(addr);
	}

	void RefSource::setPtr(const void *to) {
		set(new (this) StaticContent(to));
	}

	void RefSource::toS(StrBuf *to) const {
		*to << title();
	}


	/**
	 * NameRefSource.
	 */

	StrRefSource::StrRefSource(const wchar *title) : RefSource() {
		name = new (this) Str(title);
	}

	StrRefSource::StrRefSource(Str *title) : RefSource(), name(title) {}

	StrRefSource::StrRefSource(Str *title, Content *content) : RefSource(content), name(title) {}

	Str *StrRefSource::title() const {
		return name;
	}


}
