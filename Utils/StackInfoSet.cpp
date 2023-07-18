#include "stdafx.h"
#include "StackInfoSet.h"
#include <typeinfo>


StackInfoSet::StackInfoSet() {
	attach(cppInfo);
}

int StackInfoSet::attach(const StackInfo &lookup) {
	util::Lock::L z(lock);

	for (size_t i = 0; i < data.size(); i++) {
		const StackInfo *l = data[i];
		if (!l)
			continue;

		// We assume that if they're of the same type, they're equal.
		if (typeid(*l) == typeid(lookup))
			return -1;
	}

	for (size_t i = 0; i < data.size(); i++) {
		if (!data[i]) {
			data[i] = &lookup;
			return int(i);
		}
	}

	data.push_back(&lookup);
	return int(data.size() - 1);
}

void StackInfoSet::detach(int id) {
	util::Lock::L z(lock);

	if (id >= 0)
		data[id] = null;
}

void StackInfoSet::alloc(StackFrame *frames, nat count) {
	util::Lock::L z(lock);

	for (size_t i = 0; i < data.size(); i++) {
		if (data[i])
			data[i]->alloc(frames, count);
	}
}

void StackInfoSet::free(StackFrame *frames, nat count) {
	util::Lock::L z(lock);

	for (size_t i = 0; i < data.size(); i++) {
		if (data[i])
			data[i]->free(frames, count);
	}
}

int StackInfoSet::translate(void *ip, void *&fnBase, int &offset) {
	util::Lock::L z(lock);

	// Reverse order so that CppInfo is last.
	for (size_t i = data.size(); i > 0; i--) {
		if (data[i - 1])
			if (data[i - 1]->translate(ip, fnBase, offset))
				return int(i) - 1;
	}

	// The C++ backend will do just fine. We should not get here though.
	cppInfo.translate(ip, fnBase, offset);
	return 0;
}

void StackInfoSet::format(GenericOutput &to, int id, void *fnBase, int offset) {
	util::Lock::L z(lock);

	if (data[id]) {
		data[id]->format(to, fnBase, offset);
	} else {
		to.put("Unknown function @0x");
		to.putHex((size_t)fnBase + offset);
	}
}
