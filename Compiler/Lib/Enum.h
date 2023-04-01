#pragma once
#include "Core/Str.h"
#include "Type.h"
#include "Template.h"

namespace storm {
	STORM_PKG(core.lang);

	class EnumValue;

	// Create regular enum.
	Type *createEnum(Str *name, Size size, GcType *type);

	// Create enum usable as a bitmask.
	Type *createBitmaskEnum(Str *name, Size size, GcType *type);

	/**
	 * Type for Enums from C++.
	 */
	class Enum : public Type {
		STORM_CLASS;
	public:
		STORM_CTOR Enum(Str *name, Bool bitmask);
		Enum(Str *name, GcType *type, Bool bitmask);

		// Add new entries.
		virtual void STORM_FN add(Named *n);

		// Do we support bitwise operators?
		inline Bool STORM_FN bitmask() const { return mask; }

		// Late init.
		virtual void lateInit();

		// Generic to string for an enum.
		void CODECALL toString(StrBuf *to, Nat v);

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

	protected:
		// Load members.
		virtual Bool STORM_FN loadAll();

		// We're an integer.
		virtual code::TypeDesc *STORM_FN createTypeDesc() { return code::intDesc(engine); }

	private:
		// Shall we support bitmask operators?
		Bool mask;

		// All values contained in here.
		Array<EnumValue *> *values;

		// Create the write function required for serialization.
		Function *createWriteFn(SerializedType *type);

		// Create the read function required for serialization.
		Function *createReadFn();

		// Create the read ctor required for serialization.
		Function *createReadCtor();
	};

	/**
	 * Enum value. Represented as an inline function.
	 *
	 * TODO: Implement as a kind of variable instead?
	 */
	class EnumValue : public Function {
		STORM_CLASS;
	public:
		STORM_CTOR EnumValue(Enum *owner, Str *name, Nat value);

		// The value.
		Nat value;

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Generate code.
		void CODECALL generate(InlineParams p);
	};

	/**
	 * Provide a non-member output operator for enums.
	 */
	class EnumOutput : public Template {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR EnumOutput();

		virtual MAYBE(Named *) STORM_FN generate(SimplePart *part);
	};

}
