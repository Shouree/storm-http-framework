#include "stdafx.h"
#include "Seh.h"

#ifdef WINDOWS

#include "SafeSeh.h"
#include "Seh64.h"
#include "Code/Binary.h"
#include "Core/Str.h"
#include "Gc/Gc.h"
#include "Gc/CodeTable.h"
#include "Utils/Memory.h"
#include "Utils/StackInfoSet.h"

namespace storm {
	namespace runtime {
		storm::Gc &engineGc(Engine &e);
	}
}

namespace code {
	namespace eh {

		class SehInfo : public StackInfo {
		public:
			virtual bool translate(void *ip, void *&fnBase, int &offset) const {
				storm::CodeTable &table = storm::codeTable();
				void *code = table.find(ip);
				if (!code)
					return false;

				fnBase = code;
				offset = int((byte *)ip - (byte *)code);
				return true;
			}

			virtual void format(GenericOutput &to, void *fnBase, int offset) const {
				Binary *owner = codeBinary(fnBase);
				Str *name = owner->ownerName();
				if (name) {
					to.put(name->c_str());
				} else {
					to.put(S("<unnamed Storm function>"));
				}
			}
		};

		void activateWindowsInfo(Engine &e) {
			static RegisterInfo<SehInfo> info;
#ifdef X64
			storm::runtime::engineGc(e).setEhCallback(&exceptionCallback);
#else
			(void)e;
#endif
		}

		EXCEPTION_DISPOSITION windowsHandler(_EXCEPTION_RECORD *er, void *frame, _CONTEXT *ctx, void *dispatch) {
			PLN(L"In handler!");
			return ExceptionContinueSearch;
		}


	}
}

#endif
