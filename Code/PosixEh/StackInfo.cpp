#include "stdafx.h"
#include "StackInfo.h"
#include "Utils/StackInfoSet.h"
#include "Gc/DwarfTable.h"
#include "Code/Binary.h"
#include "LookupHook.h"

namespace code {
	namespace eh {

		/**
		 * Function lookup using the Dwarf table.
		 */

		class DwarfInfo : public StackInfo {
		public:
			virtual bool translate(void *ip, void *&fnBase, int &offset) const {
				FDE *fde = dwarfTable().find(ip);
				if (!fde)
					return false;

				byte *start = (byte *)fde->codeStart();
				if (fde->codeSize() != runtime::codeSize(start)) {
					// Something is strange...
					WARNING(L"This does not seem like code we know...");
					return false;
				}

				fnBase = start;
				offset = int((byte *)ip - start);
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

		void activatePosixInfo() {
#ifdef POSIX
			initHook();
#endif
			static RegisterInfo<DwarfInfo> info;
		}

	}
}
