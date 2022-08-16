#include "stdafx.h"
#include "AsmOut.h"
#include "Asm.h"
#include "../OpTable.h"

namespace code {
	namespace arm64 {

		// Good reference for instruction encoding:
		// https://developer.arm.com/documentation/ddi0596/2021-12/Index-by-Encoding?lang=en

		// Data processing with immediate.
		Nat instrImm() {
			return 0;
		}

		// Data processing instructions.
		Nat instrData() {
			return 0;
		}

		// Load and store instructions.
		Nat instrLoadStore() {
			return 0;
		}

		bool datOut(Output *to, Instr *instr, Instr *) {
			Operand src = instr->src();
			switch (src.type()) {
			case opLabel:
				to->putAddress(src.label());
				break;
			case opReference:
				to->putAddress(src.ref());
				break;
			case opObjReference:
				to->putObject(src.object());
				break;
			case opConstant:
				to->putSize(src.constant(), src.size());
				break;
			default:
				assert(false, L"Unsupported type for 'dat'.");
				break;
			}

			return false;
		}

		bool alignOut(Output *to, Instr *instr, Instr *) {
			to->align(Nat(instr->src().constant()));
			return false;
		}

#define OUTPUT(x) { op::x, &x ## Out }

		// This is a bit special compared to other backends:
		// To allow minimal peephole optimization when generating code, the next instruction
		// is provided as well. If "next" is non-null, then there is no label between the
		// instructions, so we can merge them. If the output chose to merge instructions,
		// the output function should return "true".
		typedef bool (*OutputFn)(Output *to, Instr *instr, MAYBE(Instr *) next);

		const OpEntry<OutputFn> outputMap[] = {
			OUTPUT(dat),
			OUTPUT(align),
		};

		bool empty(Array<Label> *x) {
			return x == null || x->empty();
		}

		void output(Listing *src, Output *to) {
			static OpTable<OutputFn> t(outputMap, ARRAY_COUNT(outputMap));

			for (Nat i = 0; i < src->count(); i++) {
				to->mark(src->labels(i));

				Instr *instr = src->at(i);
				OutputFn fn = t[instr->op()];
				if (fn) {
					Instr *next = null;
					if (i + 1 < src->count() && empty(src->labels(i + 1)))
						next = src->at(i + 1);

					if ((*fn)(to, instr, next))
						i++;
				} else {
					assert(false, L"Unsupported op-code: " + String(name(instr->op())));
				}
			}

			to->mark(src->labels(src->count()));
		}

	}
}
