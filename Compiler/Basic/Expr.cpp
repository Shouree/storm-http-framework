#include "stdafx.h"
#include "Expr.h"
#include "Core/Str.h"
#include "Compiler/Exception.h"
#include "Compiler/Type.h"

namespace storm {
	namespace bs {

		Expr::Expr(SrcPos pos) : pos(pos) {}

		ExprResult Expr::result() {
			return ExprResult();
		}

		void Expr::code(CodeGen *to, CodeResult *var) {
			assert(!var->needed());
		}

		Int Expr::castPenalty(Value to) {
			return -1;
		}

		Bool Expr::isolate() {
			return true;
		}

		SrcPos Expr::largePos() {
			return pos;
		}

		Bool Expr::thisVariable() const {
			return false;
		}

		/**
		 * Numeric literals.
		 */

		NumLiteral::NumLiteral(SrcPos pos, Int v) : Expr(pos), intValue(v), isInt(true), isSigned(true) {}

		NumLiteral::NumLiteral(SrcPos pos, Long v) : Expr(pos), intValue(v), isInt(true), isSigned(true) {}

		NumLiteral::NumLiteral(SrcPos pos, Word v) : Expr(pos), intValue(v), isInt(true), isSigned(false) {}

		NumLiteral::NumLiteral(SrcPos pos, Double v) : Expr(pos), floatValue(v), isInt(false), isSigned(true) {}

		void NumLiteral::setType(Str *suffix) {
			Str::Iter b = suffix->begin();
			if (b == suffix->end())
				throw new (this) InternalError(S("Suffixes passed to NumLiteral may not be empty."));
			Char ch = *b;
			if (++b != suffix->end())
				throw new (this) InternalError(S("Suffixes passed to NumLiteral must be exactly one character long!"));

			if (isInt) {
				switch (ch.codepoint()) {
				case 'b':
					type = StormInfo<Byte>::type(engine());
					return;
				case 'i':
					if (isSigned) {
						type = StormInfo<Int>::type(engine());
						return;
					}
					break;
				case 'n':
					type = StormInfo<Nat>::type(engine());
					return;
				case 'l':
					if (isSigned) {
						type = StormInfo<Long>::type(engine());
						return;
					}
					break;
				case 'w':
					type = StormInfo<Word>::type(engine());
					return;
				case 'f':
					if (isSigned) {
						type = StormInfo<Float>::type(engine());
						return;
					}
					break;
				case 'd':
					if (isSigned) {
						type = StormInfo<Double>::type(engine());
						return;
					}
					break;
				}
			} else {
				switch (ch.codepoint()) {
				case 'f':
					type = StormInfo<Float>::type(engine());
					return;
				case 'd':
					type = StormInfo<Double>::type(engine());
					return;
				}
			}

			throw new (this) SyntaxError(pos, TO_S(engine(), S("Invalid suffix: ") << suffix));
		}

		void NumLiteral::toS(StrBuf *to) const {
			if (isInt) {
				if (isSigned) {
					*to << intValue;
				} else {
					*to << S("0x") << hex((Word)intValue);
				}
			} else {
				*to << floatValue;
			}

			if (type)
				*to << S(" (") << type->name << S(")");
		}

		ExprResult NumLiteral::result() {
			if (type)
				return Value(type);

			if (isInt)
				if (isSigned)
					return Value(StormInfo<Int>::type(engine()));
				else
					return Value(StormInfo<Nat>::type(engine()));
			else
				return Value(StormInfo<Float>::type(engine()));
		}

		Int NumLiteral::castPenalty(Value to) {
			if (type)
				return -1;

			if (to.ref)
				return -1;

			Engine &e = engine();

			if (isInt) {
				// Prefer bigger types if multiple are possible.
				if (to.type == StormInfo<Long>::type(e) && isSigned)
					return 1;
				if (to.type == StormInfo<Word>::type(e))
					return 1;
				if (to.type == StormInfo<Int>::type(e) && (abs(intValue) & 0x7FFFFFFF) == abs(intValue) && isSigned)
					return 2;
				if (to.type == StormInfo<Nat>::type(e) && (intValue & 0xFFFFFFFF) == intValue)
					return 2;
				if (to.type == StormInfo<Byte>::type(e) && (intValue & 0xFF) == intValue)
					return 3;
				if (to.type == StormInfo<Double>::type(e) && (abs(intValue) & 0x3FFFFFFFFFFFF) == abs(intValue) && isSigned)
					// We allow up to 52 bits to automatically cast.
					return 3;
				if (to.type == StormInfo<Float>::type(e) && (abs(intValue) & 0xFFFF) == abs(intValue) && isSigned)
					// We allow up to 16 bits to automatically cast.
					return 4;
			} else {
				// We're fine with using doubles as well.
				if (to.type == StormInfo<Double>::type(e))
					return 1;
			}

			return -1;
		}

		void NumLiteral::code(CodeGen *s, CodeResult *r) {
			using namespace code;

			if (!r->needed())
				return;

			if (isInt)
				intCode(s, r);
			else
				floatCode(s, r);
		}

		void NumLiteral::intCode(CodeGen *s, CodeResult *r) {
			using namespace code;

			code::Var to = r->location(s);

			Type *t = r->type().type;
			Engine &e = engine();
			if (t == StormInfo<Int>::type(e))
				*s->l << mov(to, intConst(int(intValue)));
			else if (t == StormInfo<Nat>::type(e))
				*s->l << mov(to, natConst(nat(intValue)));
			else if (t == StormInfo<Byte>::type(e))
				*s->l << mov(to, byteConst(byte(intValue)));
			else if (t == StormInfo<Float>::type(e))
				*s->l << mov(to, floatConst(float(intValue)));
			else if (t == StormInfo<Double>::type(e))
				*s->l << mov(to, doubleConst(double(intValue)));
			else if (t == StormInfo<Long>::type(e))
				*s->l << mov(to, longConst(Long(intValue)));
			else if (t == StormInfo<Word>::type(e))
				*s->l << mov(to, wordConst(Word(intValue)));
			else
				assert(false, L"Unknown type for an integer constant.");

			r->created(s);
		}

		void NumLiteral::floatCode(CodeGen *s, CodeResult *r) {
			using namespace code;

			code::Var to = r->location(s);
			Type *t = r->type().type;
			Engine &e = engine();
			if (t == StormInfo<Float>::type(e))
				*s->l << mov(to, floatConst(float(floatValue)));
			else if (t == StormInfo<Double>::type(e))
				*s->l << mov(to, doubleConst(floatValue));
			else
				assert(false, L"Unknown type for a float constant.");

			r->created(s);
		}


		NumLiteral *intConstant(SrcPos pos, Str *v) {
			return new (v) NumLiteral(pos, v->toLong());
		}

		NumLiteral *floatConstant(SrcPos pos, Str *v) {
			return new (v) NumLiteral(pos, v->toDouble());
		}

		NumLiteral *hexConstant(SrcPos pos, Str *v) {
			return new (v) NumLiteral(pos, v->hexToWord());
		}

		/**
		 * String literals.
		 */

		StrLiteral::StrLiteral(SrcPos pos, Str *value) : Expr(pos), value(value) {}

		ExprResult StrLiteral::result() {
			return Value(StormInfo<Str>::type(engine()));
		}

		void StrLiteral::code(CodeGen *s, CodeResult *r) {
			if (!r->needed())
				return;

			// We can store the string as an object inside the code.
			code::Var to = r->location(s);
			*s->l << code::mov(to, code::objPtr(value));
			r->created(s);
		}

		void StrLiteral::toS(StrBuf *to) const {
			*to << S("\"") << value->escape(Char('"'), Char('$')) << S("\"");
		}


		StrLiteral *strConstant(syntax::SStr *v) {
			return strConstant(v->pos, v->v);
		}

		StrLiteral *strConstant(SrcPos pos, Str *v) {
			return new (v) StrLiteral(pos, v->unescape(Char('"'), Char('$')));
		}

		StrLiteral *rawStrConstant(SrcPos pos, Str *v) {
			return new (v) StrLiteral(pos, v);
		}


		/**
		 * Boolean literals.
		 */

		BoolLiteral::BoolLiteral(SrcPos pos, Bool value) : Expr(pos), value(value) {}

		ExprResult BoolLiteral::result() {
			return Value(StormInfo<Bool>::type(engine()));
		}

		void BoolLiteral::code(CodeGen *s, CodeResult *r) {
			if (!r->needed())
				return;

			code::Var to = r->location(s);
			*s->l << code::mov(to, code::byteConst(value ? 1 : 0));
			r->created(s);
		}

		void BoolLiteral::toS(StrBuf *to) const {
			*to << (value ? S("true") : S("false"));
		}


		/**
		 * Character literals.
		 */

		CharLiteral::CharLiteral(SrcPos pos, Char value) : Expr(pos), value(value) {}

		ExprResult CharLiteral::result() {
			return Value(StormInfo<Char>::type(engine()));
		}

		void CharLiteral::code(CodeGen *s, CodeResult *r) {
			if (!r->needed())
				return;

			// Note: We know that the representation of Char is simple enough to allow this even
			// though it is a class.
			code::Var to = r->location(s);
			*s->l << code::mov(to, code::natConst(value.codepoint()));
			r->created(s);
		}

		void CharLiteral::toS(StrBuf *to) const {
			Str *x = TO_S(this, value);
			*to << S("'") << x->escape(Char('\'')) << S("'");
		}

		CharLiteral *charConstant(SrcPos pos, Str *value) {
			value = value->unescape(Char('\''));

			Str::Iter x = value->begin();
			CharLiteral *result = new (value) CharLiteral(pos, x.v());
			++x;
			if (x != value->end())
				throw new (value) SyntaxError(pos, S("Expected a single character, but the supplied literal contains multiple characters."));
			return result;
		}


		/**
		 * Dummy expression.
		 */

		DummyExpr::DummyExpr(SrcPos pos, Value type) : Expr(pos), type(type) {}

		ExprResult DummyExpr::result() {
			return type;
		}

		void DummyExpr::code(CodeGen *s, CodeResult *r) {
			throw new (this) InternalError(S("Tried to generate code from a DummyExpr!"));
		}


	}
}
