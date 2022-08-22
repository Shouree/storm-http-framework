#include "stdafx.h"
#include "FnCall.h"
#include "Params.h"

namespace code {
	namespace arm64 {

		ParamInfo::ParamInfo(TypeDesc *desc, const Operand &src, Bool ref)
			: type(desc), src(src), ref(ref) {}


		// Actual entry-point.
		void emitFnCall(Listing *dest, Operand toCall, Operand resultPos, TypeDesc *resultType,
						Bool resultRef, Block currentBlock, RegSet *used, Array<ParamInfo> *params) {
			Engine &e = dest->engine();

			Params *paramLayout = new (e) Params();
			for (Nat i = 0; i < params->count(); i++)
				paramLayout->add(i, params->at(i).type);

			Result *resultLayout = result(resultType);

			TODO(L"FIXME");

		}

	}
}
