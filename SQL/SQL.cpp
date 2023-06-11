#include "stdafx.h"
#include "SQL.h"
#include "Exception.h"

namespace sql {

	/**
	 * Bind helpers.
	 */

	void bind(Statement *to, Nat pos, MAYBE(Str *) str) {
		if (str)
			to->bind(pos, str);
		else
			to->bindNull(pos);
	}

	void bind(Statement *to, Nat pos, Maybe<Bool> v) {
		if (v.any())
			to->bind(pos, v.value());
		else
			to->bindNull(pos);
	}

	void bind(Statement *to, Nat pos, Maybe<Int> v) {
		if (v.any())
			to->bind(pos, v.value());
		else
			to->bindNull(pos);
	}

	void bind(Statement *to, Nat pos, Maybe<Long> v) {
		if (v.any())
			to->bind(pos, v.value());
		else
			to->bindNull(pos);
	}

	void bind(Statement *to, Nat pos, Maybe<Float> v) {
		if (v.any())
			to->bind(pos, v.value());
		else
			to->bindNull(pos);
	}

	void bind(Statement *to, Nat pos, Maybe<Double> v) {
		if (v.any())
			to->bind(pos, v.value());
		else
			to->bindNull(pos);
	}


	/**
	 * Statement.
	 */

	Statement::Statement() : resultAliveCount(0), resultSequence(0) {}

	void Statement::invalidateResult() {
		atomicIncrement(resultSequence);
		atomicWrite(resultAliveCount, 0);
	}


	/**
	 * Statement result.
	 *
	 * Note: We use atomics to update the shared data in Statement. This is mostly to be able to be
	 * somewhat robust against the case where the API is misused from multiple threads. The
	 * implementation is not thread-safe (there are cases where we will fail), but as this is mostly
	 * a safeguard to be able to detect such cases, it is fine.
	 */

	Statement::Result::Result(Statement *stmt) : owner(stmt), sequence(atomicRead(stmt->resultSequence)) {
		atomicIncrement(owner->resultAliveCount);
	}

	Statement::Result::~Result() {
		if (atomicRead(owner->resultSequence) == sequence) {
			if (atomicDecrement(owner->resultAliveCount) == 0) {
				owner->disposeResult();
			}
		}
	}

	Statement::Result::Result(const Result &other) : owner(other.owner), sequence(other.sequence) {
		if (atomicRead(owner->resultSequence) == sequence) {
			atomicIncrement(owner->resultAliveCount);
		}
	}

	Statement::Result &Statement::Result::operator =(const Result &other) {
		if (this == &other)
			return *this;

		Result tmp(other);
		std::swap(owner, tmp.owner);
		std::swap(sequence, tmp.sequence);
		return *this;
	}

	Maybe<Row> Statement::Result::next() {
		if (atomicRead(owner->resultSequence) != sequence)
			return Maybe<Row>();

		return owner->nextRow();
	}

	Int Statement::Result::lastRowId() const {
		if (atomicRead(owner->resultSequence) != sequence)
			return -1;

		return owner->lastRowId();
	}

	Nat Statement::Result::changes() const {
		if (atomicRead(owner->resultSequence) != sequence)
			return 0;

		return owner->changes();
	}

	void Statement::Result::finalize() {
		if (atomicRead(owner->resultSequence) == sequence)
			owner->disposeResult();
	}

	void Statement::Result::deepCopy(CloneEnv *env) {
		throw new (env) SQLError(new (env) Str(S("Deep copies of database iterators are not supported.")));
	}


}
