#pragma once

namespace os {

	/**
	 * A doubly linked list that is placed inline in the data structure. Supports basic push and pop
	 * operations, and allows removing elements in the middle of the list.
	 *
	 * T must have a T *next and T * prev members that are initialized to null. Therefore, each
	 * element can not reside in more than one list at once. This is asserted in debug builds. Aside
	 * from this, elements should be comparable using <.
	 */
	template <class T>
	class SortedInlineList : NoCopy {
	public:
		// Create an empty list.
		SortedInlineList() : head(null) {}

		// Empty the list.
		~SortedInlineList() {
			while (head) {
				T *t = head;
				head = head->next;
				t->next = null;
			}
			head = null;
		}

		// Push an element to its location in the list.
		void push(T *elem) {
			assert(elem->next == null && elem->prev == null,
				L"Can not push an element into more than one list.");

			// No elements?
			if (!head) {
				head = elem;
				return;
			}

			// Insert first?
			if (!(*head < *elem)) {
				elem->next = head;
				head->prev = elem;
				head = elem;
				return;
			}

			// Find the node to insert after:
			T *insertAfter = head;
			while (insertAfter->next && *insertAfter->next < *elem)
				insertAfter = insertAfter->next;

			// Insert it:
			elem->next = insertAfter->next;
			elem->prev = insertAfter;
			if (insertAfter->next)
				insertAfter->next->prev = elem;
			insertAfter->next = elem;
		}

		// Pop an element from the beginning of the list.
		T *pop() {
			if (!head)
				return null;

			T *r = head;
			head = r->next;
			if (head)
				head->prev = null;
			r->next = null;
			r->prev = null;
			return r;
		}

		// Remove an element from anywhere in the list.
		void erase(T *elem) {
			// Update head if necessary.
			if (head == elem)
				head = elem->next;

			// Fix links:
			if (elem->prev)
				elem->prev->next = elem->next;
			if (elem->next)
				elem->next->prev = elem->prev;

			elem->next = null;
			elem->prev = null;
		}

		// Peek the topmost element.
		T *peek() {
			return head;
		}

		// Empty?
		bool empty() const {
			return head == null;
		}

		// Any?
		bool any() const {
			return head != null;
		}

		// Debug dump.
		void dbg_dump() {
			PLN(L"Head: " << head);
			for (T *x = head; x; x = x->next)
				PLN(x->prev << L" <- " << x << L" -> " << x->next);
		}

	private:
		// Head.
		T *head;
	};

}
