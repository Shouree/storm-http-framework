#include "stdafx.h"
#include "WindowsParams.h"
#include "Asm.h"

namespace code {
	namespace x64 {

		/**
		 * Windows version:
		 */

		WindowsParams::WindowsParams(Bool member) : code::Params(4, 4, 8, 16), member(member) {
			setStackExtra(32); // 4 words for shadow space.
			setCalleeDestroyParams(); // Callee's responsibility to destroy parameters.
		}

		Reg WindowsParams::registerSrc(Nat id) const {
			static Reg v[] = {
				ptrC, ptrD, ptr8, ptr9,
				xmm0, xmm1, xmm2, xmm3,
			};
			return v[id];
		}

		void WindowsParams::resultPrimitive(Primitive p) {
			if (p.kind() == primitive::none)
				resultData = Result::empty();
			else if (p.kind() == primitive::real)
				resultData = Result::inRegister(engine(), asSize(xmm0, p.size()));
			else
				resultData = Result::inRegister(engine(), asSize(ptrA, p.size()));
		}

		void WindowsParams::resultComplex(ComplexDesc *desc) {
			Nat index = member ? 1 : 0;
			addInt(index, Param(Param::returnId(), desc->size(), true, 0, true));
			resultData = Result::inMemory(registerSrc(index));
		}

		void WindowsParams::resultSimple(SimpleDesc *desc) {
			// Note: Seems like these are always passed in memory?
			Nat index = member ? 1 : 0;
			addInt(index, Param(Param::returnId(), desc->size(), true, 0, true));
			resultData = Result::inMemory(registerSrc(index));
			return;

			// Old attempt here. Might still be relevant!

			Nat size = desc->size().size64();

			if (size == 0) {
				resultData = Result::empty();
				return;
			}

			if (size > 8) {
				// Return on stack.
				Nat index = member ? 1 : 0;
				addInt(index, Param(Param::returnId(), desc->size(), true, 0, true));
				resultData = Result::inMemory(registerSrc(index));
				return;
			}

			// Put it in a register.
			resultData = Result::inRegisters(engine(), 1);
			if (size >= 8)
				resultData.putRegister(rax, 0);
			else if (size >= 4)
				resultData.putRegister(eax, 0);
			else
				resultData.putRegister(al, 0);
		}

		void WindowsParams::addPrimitive(Nat id, Primitive p) {
			switch (p.kind()) {
			case primitive::none:
				break;
			case primitive::pointer:
			case primitive::integer:
				addInt(Param(id, p, true));
				break;
			case primitive::real:
				addReal(Param(id, p, true));
				break;
			}
		}

		void WindowsParams::addComplex(Nat id, ComplexDesc *desc) {
			addInt(Param(id, desc->size(), true, 0, true));
		}

		void WindowsParams::addSimple(Nat id, SimpleDesc *desc) {
			// If they fit in a single 64-bit register, they are passed in that register.
			Nat size = desc->size().size64();

			if (size > 8) {
				addInt(Param(id, desc->size(), true, 0, true));
			} else if (size > 4) {
				addInt(Param(id, Size::sLong, true, 0, false));
			} else if (size > 1) {
				addInt(Param(id, Size::sInt, true, 0, false));
			} else {
				addInt(Param(id, Size::sByte, true, 0, false));
			}
		}

	}
}
