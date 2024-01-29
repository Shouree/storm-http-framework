#pragma once

namespace os {

	/**
	 * Defines a set implemented as a doubly-linked list within its elements. The elements are
	 * required to contain "next" and "prev" pointers of the same type as the template parameter to
	 * this class. This object is designed to keep track of a number of objects, and has therefore
	 * no support for multiple keys with the same "key". The key used in this set is the object
	 * pointer itself. The "prev" and "next" members are assumed to be initialized to zero at
	 * startup. Use the "SetMember" class to get this automatically.  The iterators in the set are
	 * designed to manage removal of the current item. Eg, iterator a = foo.begin(); foo.erase(*a);
	 * a++; is legal. Insertions during iterations will however fail in some cases.
	 *
	 * This implementation needs to be thread safe when a thread iterates through a set in the
	 * process of being modified. It is enough to be able to iterate through the entire list between
	 * any two machine instructions (disregarding any data visibility issues) since this is done by
	 * the GC when a thread to be scanned has been stopped at an arbitrary point. If iteration
	 * happens during insertion or removal, the node being modified may or may not be included in
	 * the iteration, but all other elements need to be visited by the iteration.
	 */
	template <class T>
	class InlineSet {
	public:
		// Create.
		InlineSet() : first(null), last(null), size(0) {}

		// Destroy.
		~InlineSet() {
			clear();
		}

		// Clear contents. Not thread safe.
		void clear() {
			T *current = first;
			while (current) {
				T *next = current->next;
				current->prev = null;
				current->next = null;

				current = next;
			}
			first = last = null;
			size = 0;
		}

		// Add an element to the set. It is assumed that the element has not previously been added
		// to another set. This function is thread safe wrt iterations through the set, ie. an
		// iteration through the set between any two machine instructions in this function will
		// succeed, yielding all elements in the set, possibly except for the newly inserted
		// element.
		void insert(T *item) {
			++size;

			dbg_assert(item->prev == null, L"The node being inserted is already in a set.");
			dbg_assert(item->next == null, L"The node being inserted is already in a set.");

			atomicWrite(item->prev, last);
			atomicWrite(item->next, (T *)null);

			if (last)
				atomicWrite(last->next, item);
			if (!first)
				atomicWrite(first, item);
			atomicWrite(last, item);
		}

		// Remove an element from the set. This function is thread safe wrt iterations through the
		// set, ie. an iteration through the set between any two machine instructions in this
		// function will succeed, yielding all elements in the set, possibly except for the element
		// to be removed.
		void erase(T *item) {
			dbg_assert(size > 0, L"Trying to erase elements in an empty set.");
			--size;

			if (item == first)
				atomicWrite(first, item->next);
			if (item == last)
				atomicWrite(last, item->prev);

			if (item->prev)
				atomicWrite(item->prev->next, item->next);
			if (item->next)
				atomicWrite(item->next->prev, item->prev);

			item->prev = item->next = null;
		}

		// Is the set empty?
		inline bool empty() const {
			return atomicRead(first) == null && atomicRead(last) == null;
		}

		// Any elements in the set?
		inline bool any() const {
			return !empty();
		}

		// How many elements?
		inline nat count() const {
			return size;
		}

		// Iterators...
		class iterator {
		private:
			friend class InlineSet<T>;
			iterator(T *at) : at(at), next(null) { if (at) next = atomicRead(at->next); }
			T *at;
			T *next;
		public:
			inline T *operator *() const {
				return at;
			}

			inline T *operator ->() const {
				return at;
			}

			inline operator T *() const {
				return at;
			}

			inline iterator &operator ++() {
				at = next;
				if (next)
					next = atomicRead(next->next);
				return *this;
			}

			inline iterator operator ++(int) {
				iterator t = *this;
				at = next;
				if (next)
					next = atomicRead(next->next);
				return t;
			}

			inline bool operator ==(const iterator &other) const {
				return at == other.at;
			}

			inline bool operator !=(const iterator &other) const {
				return at != other.at;
			}

			typedef std::forward_iterator_tag iterator_category;
			typedef nat difference_type;
			typedef T *value_type;
			typedef T *pointer;
			typedef T &reference;
		};

		inline iterator begin() const {
			return iterator(atomicRead(first));
		}

		inline iterator end() const {
			return iterator(null);
		}

		// See if "item" is contained within this set. This operation is slow (linear time).
		bool contains(T *item) const {
			for (iterator current = begin(); current != end(); ++current)
				if (*current == item)
					return true;
			return false;
		}

	private:
		// Disallow copying.
		InlineSet(const InlineSet &o);
		InlineSet &operator =(const InlineSet &o);

		// Data.
		T *first;
		T *last;
		nat size;
	};


	/**
	 * A simple class you can derive your data from to get a protected, correctly initialized prev
	 * and next member in your class.
	 */
	template <class T>
	class SetMember {
	public:
		SetMember() : prev(null), next(null) {}

		~SetMember() {
			// make sure we're not currently inside a set!
			dbg_assert(prev == null, L"Trying to remove a node inside a InlineSet!");
			dbg_assert(next == null, L"Trying to remove a node inside a InlineSet!");
		}

		// Check if the type is in a set.
		bool inASet() const {
			return next != null || prev != null;
		}

	private:
		friend class InlineSet<T>;
		T *prev;
		T *next;
	};

}
