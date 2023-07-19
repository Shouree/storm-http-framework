#include "stdafx.h"
#include "Resource.h"
#include "Graphics.h"
#include "Core/Exception.h"

namespace gui {

	static const Nat needUpdateFlag = 0x80000000;
	static const Nat mask = ~needUpdateFlag;

	Resource::Resource() : firstData(null), firstRefs(0), more(null) {}

	void Resource::destroy(Graphics *g) {
		// TODO: This should probably be a bit careful, as it is called inside a destructor.
		clear(g->id());
	}

	GraphicsResource *Resource::forGraphics(Graphics *g) {
		return (GraphicsResource *)forGraphicsRaw(g);
	}

	void *Resource::forGraphicsRaw(Graphics *g) {
		Bool addRef = g->attach(this);

		GraphicsMgrRaw *mgr = g->manager();

		void *ptr;
		Nat count = get(g->id(), ptr, addRef);
		if (count == 0 || ptr == null) {
			Cleanup clean;
			create(mgr, ptr, clean);
			set(g->id(), ptr, clean, addRef);
		}

		if (count & needUpdateFlag) {
			update(mgr, ptr);
		}

		return ptr;
	}

	void Resource::create(GraphicsMgrRaw *g, void *&data, Cleanup &cleanup) {
		throw new (this) NotSupported(S("Can not create this resource type."));
	}

	void Resource::update(GraphicsMgrRaw *g, void *data) {
		throw new (this) NotSupported(S("Can not update this resource type."));
	}

	void Resource::needUpdate() {
		if (firstRefs > 0)
			firstRefs |= needUpdateFlag;

		if (more) {
			for (Nat i = 0; i < more->count; i++) {
				if (more->v[i].refs)
					more->v[i].refs |= needUpdateFlag;
			}
		}
	}

	void Resource::recreate() {
		if (firstRefs > 0) {
			if (firstClean)
				(*firstClean)(firstData);
			firstData = null;
		}

		if (more) {
			for (Nat i = 0; i < more->count; i++) {
				Element &e = more->v[i];
				if (e.refs > 0) {
					if (e.clean)
						(*e.clean)(e.data);
					e.data = null;
				}
			}
		}
	}

	Nat Resource::get(Nat id, void *&result, Bool addRef) {
		if (id < offset)
			return 0;

		if (id == offset) {
			Nat r = 0;
			if (firstRefs > 0) {
				result = firstData;
				r = firstRefs;
				firstRefs = (firstRefs & mask) + Nat(addRef);
			}
			return r;
		}

		if (more && id <= offset + more->count) {
			Element &e = more->v[id - offset - 1];
			Nat r = 0;
			if (e.refs > 0) {
				result = e.data;
				r = e.refs;
				e.refs = (e.refs & mask) + Nat(addRef);
			}
			return r;
		}

		return 0;
	}

	const GcType Resource::elemType = {
		GcType::tArray,
		null,
		null,
		sizeof(Resource::Element),
		1,
		{ OFFSET_OF(Resource::Element, data) }
	};

	void Resource::resize(Nat rangeMin, Nat rangeMax) {
		Nat currMin = offset;
		Nat currMax = offset;
		if (more)
			currMax = Nat(more->count + 1);
		else if (firstRefs)
			currMax = offset + 1;

		// Nothing to do?
		if (currMin == rangeMin && currMax == rangeMax)
			return;

		// Only zero or one element?
		if (rangeMin == rangeMax) {
			firstRefs = 0;
			offset = 0;
			more = null;
			return;
		} else if (rangeMin + 1 == rangeMax) {
			firstRefs = get(rangeMin, firstData, false);
			offset = rangeMin;
			more = null;
			return;
		}

		GcArray<Element> *newMore = runtime::allocArray<Element>(engine(), &elemType, rangeMax - rangeMin - 1);
		if (currMin == rangeMin) {
			// Same first element. Then we only need to copy the remaining elements.
			for (Nat i = currMin + 1; i < min(currMax, rangeMax); i++) {
				newMore->v[i - offset - 1] = more->v[i - offset - 1];
			}
		} else if (currMin < rangeMin) {
			// The current first element will be removed. Just copy it from the array.
			firstData = more->v[rangeMin - offset].data;
			firstRefs = more->v[rangeMin - offset].refs;

			for (Nat i = rangeMin + 1; i < min(currMax, rangeMax); i++) {
				newMore->v[i - rangeMin - 1] = more->v[i - offset - 1];
			}
		} else {
			// We need to put the new element in the new array somewhere. It will be blank for now.
			newMore->v[currMin - rangeMin - 1].data = firstData;
			newMore->v[currMin - rangeMin - 1].refs = firstRefs;

			for (Nat i = currMin + 1; i < min(currMax, rangeMax); i++) {
				newMore->v[i - rangeMin - 1] = more->v[i - offset - 1];
			}

			firstData = null;
			firstRefs = 0;
		}

		more = newMore;
		offset = rangeMin;
	}

	Nat Resource::set(Nat id, void *data, Cleanup clean, Bool addRef) {
		// Empty?
		if (firstRefs == 0) {
			firstRefs = Nat(addRef);
			firstData = data;
			firstClean = clean;
			offset = id;
			return firstRefs;
		}

		// Make sure we have space.
		Nat rangeMin = min(offset, id);
		Nat rangeMax = max(offset, id) + 1;
		if (more)
			rangeMax = max(rangeMax, Nat(more->count) + 1);

		resize(rangeMin, rangeMax);

		if (offset == id) {
			// First element.
			firstData = data;
			firstClean = clean;
			firstRefs += Nat(addRef);
			return firstRefs;
		} else {
			// Must be one of the remaining ones.
			Element &e = more->v[id - offset - 1];
			e.data = data;
			e.clean = clean;
			e.refs += Nat(addRef);
			return e.refs;
		}
	}

	void Resource::shrink() {
		Bool found = false;
		Nat low = 0;
		Nat high = 0;

		if (firstRefs > 0) {
			found = true;
			low = offset;
			high = offset + 1;
		}

		if (more) {
			for (Nat i = 0; i < more->count; i++) {
				if (more->v[i].refs > 0) {
					if (!found)
						low = i + offset;
					found = true;
					high = i + offset + 1;
				}
			}
		}

		resize(low, high);
	}

	void Resource::clear(Nat id) {
		if (id < offset)
			return;

		if (id == offset) {
			if (firstRefs == 0)
				return;
			if (((--firstRefs) & mask) > 0)
				return;

			firstRefs = 0;
			if (firstClean)
				(*firstClean)(firstData);
			firstData = null;
			shrink();
		} else if (more && id <= offset + more->count) {
			Element &e = more->v[id - offset - 1];
			if (e.refs == 0)
				return;
			if (((--e.refs) & mask) > 0)
				return;

			e.refs = 0;
			if (e.clean)
				(*e.clean)(e.data);
			e.data = null;
			shrink();
		}
	}

}
