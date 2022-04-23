#include "stdafx.h"
#include "EngineFwd.h"
#include "Runtime.h"
#include "OS/Shared.h"
#include "Utils/StackInfoSet.h"

namespace storm {

	const EngineFwdShared &engineFwd() {
		static const EngineFwdShared fwd = {
			&runtime::typeHandle,
			&runtime::voidHandle,
			&runtime::refObjHandle,
			&runtime::typeOf,
			&runtime::gcTypeOf,
			&runtime::typeGc,
			&runtime::typeName,
			&runtime::typeIdentifier,
			&runtime::fromIdentifier,
			&runtime::isValue,
			&runtime::isA,
			&runtime::isA,
			&runtime::allocEngine,
			&runtime::allocRaw,
			&runtime::allocStaticRaw,
			&runtime::allocBuffer,
			&runtime::allocObject,
			&runtime::allocArray,
			&runtime::allocWeakArray,
			&runtime::allocCode,
			&runtime::codeSize,
			&runtime::codeRefs,
			&runtime::codeUpdatePtrs,
			&runtime::setVTable,
			&runtime::liveObject,
			&runtime::threadGroup,
			&runtime::threadLock,
			&runtime::createWatch,
			&runtime::postStdRequest,
			&runtime::cloneObject,
			&runtime::cloneObjectEnv,
			&runtime::someEngineUnsafe,

			// Others.
			&os::currentThreadData,
			&os::currentThreadData,
			&os::currentUThreadState,
			&os::currentUThreadState,
			&os::threadCreated,
			&os::threadTerminated,

			&::stackInfo,

			// Dummy.
			1.0f,
		};

		return fwd;
	}

	namespace runtime {
		Engine &someEngine() {
			Engine *e = someEngineUnsafe();
			assert(e, L"Thread was not associated with an engine!");
			return *e;
		}
	}
}
