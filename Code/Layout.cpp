#include "stdafx.h"
#include "Layout.h"

namespace code {

	static Offset addVar(Listing *src, Array<Offset> *db, Array<Bool> *valid, Var v, const Size &minAlign) {
		// Parameter?
		if (src->isParam(v))
			return Offset();

		// Invalid?
		if (v == Var())
			return Offset();

		Nat id = v.key();

		// We've got something useful!
		if (valid->at(id))
			return db->at(id);

		Var prevVar = src->prev(v);
		Offset offset = addVar(src, db, valid, prevVar, minAlign);

		if (!src->isParam(prevVar))
			offset += prevVar.size().aligned();

		// Align 'prev' to something useful.
		offset = offset.alignAs(minAlign).alignAs(v.size());

		valid->at(id) = true;
		db->at(id) = offset;

		return offset;
	}

	Array<Offset> *layout(Listing *src) {
		return layout(src, Size::sByte);
	}

	Array<Offset> *layout(Listing *src, Size minAlign) {
		Array<Var> *all = src->allVars();

		Array<Offset> *result = new (src) Array<Offset>(all->count() + 1, Offset());
		Array<Bool> *populated = new (src) Array<Bool>(all->count(), false);

		Offset worst;

		for (nat i = 0; i < all->count(); i++) {
			Offset o = addVar(src, result, populated, all->at(i), minAlign);
			o = (o + all->at(i).size());
			// Align to the most restrictive of pointer alignment and the alignment of the type.
			o = o.alignAs(Size::sPtr).alignAs(all->at(i).size());

			worst = max(worst, o);
		}

		result->last() = worst;
		return result;
	}

}
