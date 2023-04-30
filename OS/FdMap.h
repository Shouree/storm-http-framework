#pragma once

#include <iomanip>

#if defined(POSIX)
#include <poll.h>
#endif

namespace os {

#ifndef WINDOWS

	/**
	 * A map for file descriptors (mainly relevant on UNIX-like systems).
	 *
	 * Maintains a map from file descriptor (an int) to some user-defined data (a pointer). Also
	 * maintains an array of 'struct pollfd' that describes the contents. Allows quick lookups from
	 * fd index to data in addition to lookups from the key.
	 *
	 * The same key may be used multiple times.
	 *
	 * 'unused' tells how many unused elements the array of 'pollfd' structs shall have in the beginning.
	 */
	template <class T, size_t unused>
	class FdMap {
	public:
		// Create.
		FdMap();

		// Destroy.
		~FdMap();

		// Number of elements.
		nat count() const { return elems; }

		// Current capacity.
		nat capacity() const { return size; }

		// Add an element.
		void put(int fd, short events, T *value);
		void put(const struct pollfd &k, T *value);

		// Find an element. Returns >= capacity() if not found.
		nat find(int fd);

		// Get the next element with the same key as 'pos'. Returns >= capacity() if not found.
		nat next(nat pos);

		// Remove an element.
		void remove(nat pos);

		// Get the value for element at 'pos'.
		T *valueAt(nat pos) const { return val[pos]; }

		// Get the data. May change if the FdMap is altered. Contains 'capacity + unused' elements.
		struct pollfd *data() const { return key; }

		// Print the contents for debugging.
		void dbg_print();

	private:
		FdMap(const FdMap &);
		FdMap &operator =(const FdMap &);

		// # of elements.
		nat elems;

		// capacity
		nat size;

		// Last known free slot.
		nat lastFree;

		// Special values for element information.
		static const nat free = -1;
		static const nat end = -2;

		// Information about each element.
		nat *info;

		// Keys. -1 indicates 'unused'.
		struct pollfd *key;

		// Values.
		T **val;

		// Grow if necessary.
		void grow();

		// Shrink if necessary.
		void shrink();

		// Rehash to 'n' elements.
		void rehash(nat n);

		// Hash a fd.
		static nat hash(int v) {
			v = (v ^ 0xDEADBEEF) + (v << 4);
			v = v ^ (v >> 10);
			v = v + (v << 7);
			v = v ^ (v >> 13);
			return v;
		}

		// Compute the primary slot for an fd.
		nat primarySlot(nat fd) {
			// Note: capacity will be a power of 2.
			return hash(fd) & (capacity() - 1);
		}

		// Find a free slot.
		nat freeSlot() {
			while (info[lastFree] != free)
				// Note: capacity is a power of 2
				lastFree = (lastFree + 1) & (size - 1);

			return lastFree;
		}
	};

	template <class T, size_t unused>
	FdMap<T, unused>::FdMap() : elems(0), size(0), lastFree(0), info(null), key(null), val(null) {
		if (unused)
			key = new struct pollfd[unused];
	}

	template <class T, size_t unused>
	FdMap<T, unused>::~FdMap() {
		delete []info;
		delete []key;
		delete []val;
	}

	template <class T, size_t unused>
	void FdMap<T, unused>::put(int fd, short events, T *value) {
		struct pollfd p = { fd, events, 0 };
		put(p, value);
	}

	template <class T, size_t unused>
	void FdMap<T, unused>::put(const struct pollfd &fd, T *value) {
		grow();

		nat insert = end;
		nat into = primarySlot(fd.fd);
		if (info[into] != free) {
			// Check if the contained element is in its primary position.
			nat from = primarySlot(key[into + unused].fd);
			if (from == into) {
				// It is in its primary position. Attach ourselves to the chain.
				nat to = freeSlot();
				insert = info[into];
				info[into] = to;
				into = to;
			} else {
				// It is not. Move it somewhere else.

				// Walk the list from the original position and find the node before the one we're about to move...
				while (info[from] != into)
					from = info[from];

				// Redo linking.
				nat to = freeSlot();
				info[to] = info[into];
				key[to + unused] = key[into + unused];
				val[to] = val[into];
				info[into] = free;
			}
		}

		assert(info[into] == free);
		info[into] = insert;
		key[into + unused] = fd;
		val[into] = value;
		elems++;
	}

	template <class T, size_t unused>
	nat FdMap<T, unused>::find(int fd) {
		if (capacity() == 0)
			return free;

		nat slot = primarySlot(fd);
		if (info[slot] == free)
			return free;

		do {
			if (key[slot + unused].fd == fd)
				return slot;

			slot = info[slot];
		} while (slot != end);

		return free;
	}

	template <class T, size_t unused>
	nat FdMap<T, unused>::next(nat pos) {
		int fd = key[pos + unused].fd;
		nat slot = info[pos];
		while (slot != end) {
			if (key[slot + unused].fd == fd)
				return slot;

			slot = info[pos];
		}

		return free;
	}

	template <class T, size_t unused>
	void FdMap<T, unused>::remove(nat pos) {
		int fd = key[pos + unused].fd;
		nat slot = primarySlot(fd);

		if (info[slot] == free)
			return;

		nat prev = free;
		do {
			if (slot == pos) {
				// This is the one!
				if (prev != free) {
					// Unlink from the chain.
					info[prev] = info[slot];
				}

				nat next = info[slot];

				// Destroy the node.
				info[slot] = free;
				key[slot + unused].fd = -1;
				val[slot] = null;

				if (prev == free && next != end) {
					// The removed node was in the primary slot, and we need to move the next one into our slot.
					info[slot] = info[next];
					info[next] = free;

					key[slot + unused] = key[next + unused];
					key[next + unused].fd = -1;

					val[slot] = val[next];
					val[next] = null;
				}

				elems--;
				shrink();
				return;
			}

			prev = slot;
			slot = info[slot];
		} while (slot != end);
	}

	template <class T, size_t unused>
	void FdMap<T, unused>::grow() {
		if (size == 0) {
			rehash(8);
		} else if (size == elems) {
			rehash(size * 2);
		}
	}

	template <class T, size_t unused>
	void FdMap<T, unused>::shrink() {
		if (size <= 8)
			return;

		if (elems * 3 <= size)
			rehash(size / 2);
	}

	template <class T, size_t unused>
	void FdMap<T, unused>::rehash(nat n) {
		nat oldSize = size;
		nat *oldInfo = info;
		struct pollfd *oldKey = key;
		T **oldVal = val;

		info = new nat[n];
		key = new struct pollfd[unused + n];
		val = new T *[n];
		size = n;
		lastFree = 0;

		// Initialize.
		for (nat i = 0; i < size; i++) {
			info[i] = free;
			key[i + unused].fd = -1;
			val[i] = null;
		}

		if (oldInfo) {
			// Insert all elements again.
			for (nat i = 0; i < oldSize; i++) {
				if (oldInfo[i] == free)
					continue;

				struct pollfd &k = oldKey[i + unused];
				put(k, val[i]);
			}
		}

		delete []oldInfo;
		delete []oldKey;
		delete []oldVal;
	}

	template <class T, size_t unused>
	void FdMap<T, unused>::dbg_print() {
		std::wcout << L"Map contents:" << endl;
		for (nat i = 0; i < capacity(); i++) {
			std::wcout << std::setw(2) << i << L": ";
			if (info[i] == free) {
				std::wcout << L"free";
			} else if (info[i] == end) {
				std::wcout << L"end";
			} else {
				std::wcout << L" -> " << info[i];
			}

			if (info[i] != free) {
				std::wcout << "  \t";
				std::wcout << std::setw(2) << key[i + unused].fd << "\t" << val[i];
			}
			std::wcout << endl;
		}
	}

#endif

}
