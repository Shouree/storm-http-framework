#include "stdafx.h"
#include "Access.h"
#include "Engine.h"
#include "Exception.h"
#include "Class.h"
#include "Function.h"
#include "Scope.h"
#include "Content.h"

namespace storm {
	namespace bs {

		/**
		 * Convenience functions.
		 */

		Visibility *typePublic(EnginePtr e) {
			return e.v.visibility(Engine::vPublic);
		}

		Visibility *typeProtected(EnginePtr e) {
			return e.v.visibility(Engine::vTypeProtected);
		}

		Visibility *typePackage(EnginePtr e) {
			return e.v.visibility(Engine::vPackagePrivate);
		}

		Visibility *typePrivate(EnginePtr e) {
			return e.v.visibility(Engine::vTypePrivate);
		}

		Visibility *freePublic(EnginePtr e) {
			return e.v.visibility(Engine::vPublic);
		}

		Visibility *freePackage(EnginePtr e) {
			return e.v.visibility(Engine::vPackagePrivate);
		}

		Visibility *freePrivate(EnginePtr e) {
			return e.v.visibility(Engine::vFilePrivate);
		}


		Named *apply(SrcPos pos, Named *to, Visibility *v) {
			if (to->visibility) {
				Str *msg = TO_S(to, to->name << S(" already has a visibility specified. Can not add ")
								<< v << S(" as well."));
				throw new (to) SyntaxError(pos, msg);
			}

			to->visibility = v;
			return to;
		}

		NamedDecl *apply(SrcPos pos, NamedDecl *to, Visibility *v) {
			if (to->visibility) {
				Str *msg = TO_S(to, S("The declaration ") << to
								<< S(" already has a visibility specified. Can not add ")
								<< v << S(" as well."));
				throw new (to) SyntaxError(pos, msg);
			}

			to->visibility = v;
			return to;
		}

		MemberWrap *apply(SrcPos pos, MemberWrap *to, Visibility *v) {
			if (to->visibility)
				throw new (to) SyntaxError(pos, S("This member already has a visibility specified. Can not add another."));

			to->visibility = v;
			return to;
		}

		MultiDecl *apply(SrcPos pos, MultiDecl *to, Visibility *v) {
			for (Nat i = 0; i < to->data->count(); i++)
				apply(pos, to->data->at(i), v);
			return to;
		}

		TObject *apply(SrcPos pos, TObject *to, Visibility *v) {
			if (Named *n = as<Named>(to))
				return apply(pos, n, v);
			else if (NamedDecl *d = as<NamedDecl>(to))
				return apply(pos, d, v);
			else if (MemberWrap *wrap = as<MemberWrap>(to))
				return apply(pos, wrap, v);
			else if (MultiDecl *multi = as<MultiDecl>(to))
				return apply(pos, multi, v);
			else
				throw new (to) InternalError(TO_S(to, S("I can not apply visibility to ") << to));
		}

	}
}
