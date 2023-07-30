#include "stdafx.h"
#include "Debug.h"

// For debugging!
#include "Utils/TIB.h"
#include "Code/WindowsEh/SafeSeh.h"

namespace code {

#if defined(X86) && defined(WINDOWS)

	struct SehFrame {
		SehFrame *prev;
		void *fn;

		// From the storm code generation.
		void *block, *part;
	};

	void dumpStack() {
		void *stackPtr;
		SehFrame *top;
		__asm {
			mov eax, fs:[0];
			mov top, eax;
			mov stackPtr, esp;
		};

		PLN("---- STACK DUMP ----");

		NT_TIB *tib = getTIB();
		PLN("Stack base: " << tib->StackBase);
		PLN("Stack top: " << tib->StackLimit);
		PLN("Stack ptr: " << stackPtr);
		PLN("Stack size: " << (nat(tib->StackBase) - nat(tib->StackLimit)));
		if (stackPtr > tib->StackBase)
			PLN("Above stack.");
		if (stackPtr < tib->StackLimit)
			PLN("Below stack.");

		for (nat i = 0; i < 10; i++) {
			PLN("SEH at: " << top);
			nat addr = nat(top);
			if (addr == null || addr == 0xFFFFFFFF)
				break;

			PLN("Handler: " << top->fn << " (storm: " << &x86SafeSEH << ")");
			if (nat(top) % 4 != 0)
				PLN("ERROR: STACK IS NOT WORD-ALIGNED");

			if (top->fn == &x86SafeSEH) {
				void **ebp = (void **)(top + 1);
				PLN("Ebp: " << ebp);

				for (nat p = 0; p < 3; p++)
					PLN("Param " << p << ": " << ebp[2 + p]);
				for (nat v = 0; v < 5; v++)
					PLN("Local " << v << ": " << ebp[-5 - v]);
			}

			top = top->prev;
		}

		PLN("---- DONE ----");
	}

#else

	void dumpStack() {
		PLN("Stack dumping is not implemented for your system yet!");
		PLN("See Code/Debug.h for where to implement it.");
	}

#endif

}
