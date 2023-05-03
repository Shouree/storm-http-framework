#include "stdafx.h"
#include "SQL.h"
#include "Core/Exception.h"

namespace sql {
	/////////////////////////////////////
	//			   Statement		  //
	/////////////////////////////////////

	Statement::Statement() : iteratorLive(0), iteratorGen(0) {}

	void Statement::invalidateIterators() {
		atomicIncrement(iteratorGen);

		Nat liveIters = atomicRead(iteratorLive);
		if (liveIters) {
			WARNING(liveIters << L" iterators to a query were live, but are invalidated.");
		}
		atomicWrite(iteratorLive, 0);
	}

	Statement::Iter::Iter(Statement *stmt) : owner(stmt), gen(atomicRead(stmt->iteratorGen)) {
		atomicIncrement(owner->iteratorLive);
	}

	Statement::Iter::Iter(const Iter &other) : owner(other.owner), gen(other.gen) {
		// Note: This will not work perfectly when using multiple threads. This is not expected to
		// happen (other things will break then), but the atomics are mostly a safeguard to detect
		// such errors.
		if (atomicRead(owner->iteratorGen) == gen)
			atomicIncrement(owner->iteratorLive);
	}

	Statement::Iter &Statement::Iter::operator =(const Iter &other) {
		Iter tmp(other);
		std::swap(owner, tmp.owner);
		return *this;
	}

	Statement::Iter::~Iter() {
		if (atomicRead(owner->iteratorGen) == gen) {
			if (atomicDecrement(owner->iteratorLive) == 0) {
				owner->done();
			}
		}
	}

	Row *Statement::Iter::next() {
		if (atomicRead(owner->iteratorGen) == gen)
			return owner->fetch();
		else
			return null;
	}

	Statement::Iter Statement::iter() {
		return Iter(this);
	}

	MAYBE(Row *) Statement::fetchOne() {
		Row *result = fetch();
		done();
		return result;
	}

	/////////////////////////////////////
	//				 Row			   //
	/////////////////////////////////////

	Row::Row() : v(null) {}

	Row::Row(Array<Variant> * v) : v(v) {}

	Str* Row::getStr(Nat idx) {
		if (v) {
			Variant z = v->at(idx);
			if (z.empty())
				return new (this) Str();
			else
				return z.get<Str *>();
		} else {
			throw new (this) ArrayError(0, 0);
		}
	}

	Int Row::getInt(Nat idx) {
		if (v) {
			Variant z = v->at(idx);
			if (z.empty())
				return 0;
			else
				return (Int)z.get<Long>();
		} else {
			throw new (this) ArrayError(0, 0);
		}
	}

	Long Row::getLong(Nat idx) {
		if (v) {
			Variant z = v->at(idx);
			if (z.empty())
				return 0;
			else
				return z.get<Long>();
		} else {
			throw new (this) ArrayError(0, 0);
		}
	}

	Double Row::getDouble(Nat idx) {
		if (v) {
			Variant z = v->at(idx);
			if (z.empty())
				return 0;
			else
				return (Int)z.get<Double>();
		} else {
			throw new (this) ArrayError(0, 0);
		}
	}

	Bool Row::isNull(Nat idx) {
		if (v) {
			return v->at(idx).empty();
		} else {
			throw new (this) ArrayError(0, 0);
		}
	}

}
