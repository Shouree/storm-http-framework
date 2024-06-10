#pragma once

namespace storm {
	namespace syntax {
		namespace glr {
			STORM_PKG(lang.bnf.glr);

			/**
			 * Store parent requirements for a parser state efficiently as a bitset.
			 *
			 * We expect there to be a fairly small number of requirements in the grammar, so we
			 * give productions used as requirements their own identifiers in this remark to keep
			 * the representation compact. This class stores a set of these identifiers and has
			 * support for operations on them.
			 *
			 * We also expect that most operations done by the parser do not modify these sets, and
			 * therefore the representation is immutable so that we can easily share the backing
			 * storage of the representation without creating copies and thereby adding pressure on
			 * the GC.
			 */
			class ParentReq {
				STORM_VALUE;
			public:
				// Create an empty requirement set (no allocations required).
				ParentReq();

				// Create a requirement set with one element set.
				ParentReq(Engine &e, Nat elem);

				// Empty?
				Bool STORM_FN empty() const { return data == null; }
				Bool STORM_FN any() const { return data != null; }

				// Count the number of set bits.
				Nat STORM_FN count() const;

				// Maximum possible set bit. May be larger than the actual maximum.
				Nat STORM_FN max() const;

				// Get a particular bit.
				Bool STORM_FN get(Nat id) const;

				// Create the union of two requirements.
				ParentReq concat(Engine &e, ParentReq other) const;

				// Remove elements from another ParentReq.
				ParentReq remove(Engine &e, ParentReq other) const;

				// Does this one contain all elements in 'other'?
				Bool has(ParentReq other) const;

				// Compare.
				Bool STORM_FN operator ==(const ParentReq &o) const;

				// Output.
				void STORM_FN toS(StrBuf *to) const;

			private:
				// Create, explicitly specifying the data.
				ParentReq(GcArray<Nat> *data);

				// The actual data. Large enough to store all elements, but not larger than that. If
				// no bits are set, this is null.
				GcArray<Nat> *data;
			};

			wostream &operator <<(wostream &to, const ParentReq &r);

		}
	}
}
