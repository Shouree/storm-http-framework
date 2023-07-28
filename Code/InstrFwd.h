#pragma once

namespace code {

	/**
	 * Make these functions more convenient to call in C++ during certain conditions:
	 *
	 * It inhibits the clarity and it is a bit annoying to have to do:
	 * *foo << mov(e, eax, ebx);
	 *
	 * instead of:
	 * *foo << mov(eax, ebx);
	 *
	 * The first engine parameter is not visible in Storm (as it will insert that for us
	 * automatically), but in C++ we need another mechanism as C++ does not know which engine we
	 * will use. So in the context of directly appending instructions to a Listing object, we can
	 * save the parameters in some container temporarily and delay the call to the actual creating
	 * function until the result is appended to a Listing. When this happens, we can get an Engine
	 * from there and create the Instr object properly. This is done with the template magic
	 * below. Sadly, we need to declare all machine operations once more at the end, but that is a
	 * small price to pay.
	 */

	template <Instr *(*Fn)(EnginePtr)>
	class InstrProxy0 {};

	template <Instr *(*Fn)(EnginePtr)>
	inline Listing &operator <<(Listing &l, const InstrProxy0<Fn> &p) {
		return l << (*Fn)(l.engine());
	}


	template <class T, Instr *(*Fn)(EnginePtr, T)>
	class InstrProxy1 {
	public:
		const T &t;
		InstrProxy1(const T &t) : t(t) {}
	};

	template <class T, Instr *(*Fn)(EnginePtr, T)>
	inline Listing &operator <<(Listing &l, const InstrProxy1<T, Fn> &p) {
		return l << (*Fn)(l.engine(), p.t);
	}


	template <class T, class U, Instr *(*Fn)(EnginePtr, T, U)>
	class InstrProxy2 {
	public:
		const T &t;
		const U &u;
		InstrProxy2(const T &t, const U &u) : t(t), u(u) {}
	};

	template <class T, class U, Instr *(*Fn)(EnginePtr, T, U)>
	inline Listing &operator <<(Listing &l, const InstrProxy2<T, U, Fn> &p) {
		return l << (*Fn)(l.engine(), p.t, p.u);
	}


	template <class T, class U, class V, Instr *(*Fn)(EnginePtr, T, U, V)>
	class InstrProxy3 {
	public:
		const T &t;
		const U &u;
		const V &v;
		InstrProxy3(const T &t, const U &u, const V &v) : t(t), u(u), v(v) {}
	};

	template <class T, class U, class V, Instr *(*Fn)(EnginePtr, T, U, V)>
	inline Listing &operator <<(Listing &l, const InstrProxy3<T, U, V, Fn> &p) {
		return l << (*Fn)(l.engine(), p.t, p.u, p.v);
	}

	template <class T, class U, class V, class W, Instr *(*Fn)(EnginePtr, T, U, V, W)>
	class InstrProxy4 {
	public:
		const T &t;
		const U &u;
		const V &v;
		const W &w;
		InstrProxy4(const T &t, const U &u, const V &v, const W &w) : t(t), u(u), v(v), w(w) {}
	};

	template <class T, class U, class V, class W, Instr *(*Fn)(EnginePtr, T, U, V, W)>
	inline Listing &operator <<(Listing &l, const InstrProxy4<T, U, V, W, Fn> &p) {
		return l << (*Fn)(l.engine(), p.t, p.u, p.v, p.w);
	}


#define PROXY0(op)												\
	inline InstrProxy0<&op> op() {								\
		return InstrProxy0<&op>();								\
	}
#define PROXY1(op, T)													\
	inline InstrProxy1<T, &op> op(T const &t) {							\
		return InstrProxy1<T, &op>(t);									\
	}
#define PROXY2(op, T, U)												\
	inline InstrProxy2<T, U, &op> op(T const &t, U const &u) {			\
		return InstrProxy2<T, U, &op>(t, u);							\
	}
#define PROXY3(op, T, U, V)												\
	inline InstrProxy3<T, U, V, &op> op(T const &t, U const &u, V const &v) { \
		return InstrProxy3<T, U, V, &op>(t, u, v);						\
	}
#define PROXY4(op, T, U, V, W)											\
	inline InstrProxy4<T, U, V, W, &op> op(T const &t, U const &u, V const &v, W const &w) { \
		return InstrProxy4<T, U, V, W, &op>(t, u, v, w);				\
	}

	// Repetition of all OP-codes.
	PROXY2(mov, Operand, Operand);
	PROXY2(swap, Reg, Operand);
	PROXY1(push, Operand);
	PROXY1(pop, Operand);
	PROXY0(pushFlags);
	PROXY0(popFlags);
	PROXY1(jmp, Operand);
	PROXY2(jmp, Label, CondFlag);
	PROXY2(call, Operand, Size);
	PROXY1(ret, Size);
	PROXY2(lea, Operand, Operand);
	PROXY2(setCond, Operand, CondFlag);
	PROXY2(fnParam, TypeDesc *, Operand);
	PROXY2(fnParamRef, TypeDesc *, Operand);
	PROXY2(fnCall, Operand, Bool);
	PROXY4(fnCall, Operand, Bool, TypeDesc *, Operand);
	PROXY4(fnCallRef, Operand, Bool, TypeDesc *, Operand);
	PROXY1(fnRet, Operand);
	PROXY1(fnRetRef, Operand);
	PROXY0(fnRet);
	PROXY2(bor, Operand, Operand);
	PROXY2(band, Operand, Operand);
	PROXY2(bxor, Operand, Operand);
	PROXY1(bnot, Operand);
	PROXY2(test, Operand, Operand);
	PROXY2(add, Operand, Operand);
	PROXY2(adc, Operand, Operand);
	PROXY2(sub, Operand, Operand);
	PROXY2(sbb, Operand, Operand);
	PROXY2(cmp, Operand, Operand);
	PROXY2(mul, Operand, Operand);
	PROXY2(idiv, Operand, Operand);
	PROXY2(imod, Operand, Operand);
	PROXY2(udiv, Operand, Operand);
	PROXY2(umod, Operand, Operand);
	PROXY2(shl, Operand, Operand);
	PROXY2(shr, Operand, Operand);
	PROXY2(sar, Operand, Operand);
	PROXY2(icast, Operand, Operand);
	PROXY2(ucast, Operand, Operand);
	PROXY2(fadd, Operand, Operand);
	PROXY2(fsub, Operand, Operand);
	PROXY2(fneg, Operand, Operand);
	PROXY2(fmul, Operand, Operand);
	PROXY2(fdiv, Operand, Operand);
	PROXY2(fcmp, Operand, Operand);
	PROXY2(fcast, Operand, Operand);
	PROXY2(fcasti, Operand, Operand);
	PROXY2(fcastu, Operand, Operand);
	PROXY2(icastf, Operand, Operand);
	PROXY2(ucastf, Operand, Operand);
	PROXY1(fstp, Operand);
	PROXY1(fld, Operand);
	PROXY1(dat, Operand);
	PROXY1(lblOffset, Label);
	PROXY1(align, Offset);
	PROXY1(alignAs, Size);
	PROXY0(prolog);
	PROXY0(epilog);
	PROXY2(preserve, Operand, Reg);
	PROXY1(preserve, Reg);
	PROXY1(location, SrcPos);
	PROXY1(begin, Block);
	PROXY1(end, Block);
	PROXY2(jmpBlock, Label, Block);
	PROXY1(activate, Var);
	PROXY0(threadLocal);
}
