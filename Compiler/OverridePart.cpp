#include "stdafx.h"
#include "OverridePart.h"
#include "Function.h"
#include "Type.h"

namespace storm {

	// NOTE: Slightly dangerous to re-use the parameters from the function...
	OverridePart::OverridePart(Function *src) : SimplePart(src->name, src->params), result(src->result) {}

	OverridePart::OverridePart(Type *parent, Function *src) :
		SimplePart(src->name, new (src) Array<Value>(*src->params)),
		result(src->result) {

		params->at(0) = thisPtr(parent);
	}

	Int OverridePart::matches(Named *candidate, Scope scope) const {
		Function *fn = as<Function>(candidate);
		if (!fn)
			return -1;

		Array<Value> *c = fn->params;
		if (c->count() != params->count())
			return -1;

		if (c->count() < 1)
			return -1;

		// The penalty of this match. It is only necessary in case #2 where 'candidate' is a
		// superclass wrt us, since there may be multiple functions in the parent class that may be
		// applicable in the parent, eg. if we're accepting Object or a similarly general type.
		// Currently, penalty will either be 0 or 1, with zero meaning 'exact match' and one meaning
		// 'inexact match'. In practice, this means that a function in a subclass may override in a
		// wider scope if there is no ambiguity. If multiple options exists, one has to match exactly,
		// otherwise we will bail out with a 'multiple possible matches' message.
		Int penalty = 0;

		if (params->at(0).mayReferTo(c->at(0))) {
			// Candidate is in a subclass wrt us.
			for (nat i = 1; i < c->count(); i++) {
				// Candidate needs to accept wider inputs than us.
				if (!c->at(i).mayReferTo(params->at(i)))
					return -1;
			}
		} else if (c->at(0).mayReferTo(params->at(0))) {
			// Candidate is in a superclass wrt us.
			for (nat i = 1; i < c->count(); i++) {
				// We need to accept wider inputs than candidate.
				if (!params->at(i).mayReferTo(c->at(i)))
					return -1;

				// See if it was an exact match or not. There is no scale here, only 'match' and 'no match'.
				if (c->at(i).type != params->at(i).type)
					penalty = 1;
			}
		} else {
			return -1;
		}

		return penalty;
	}

	void OverridePart::toS(StrBuf *to) const {
		*to << S("(vtable lookup) ");
		SimplePart::toS(to);
	}

}
