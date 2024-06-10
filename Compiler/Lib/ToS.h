#pragma once
#include "Compiler/Template.h"
#include "Compiler/Function.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Provide a system-wide toS function for value types that only implement toS(StrBuf).
	 */
	class ToSTemplate : public Template {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ToSTemplate();

		virtual MAYBE(Named *) STORM_FN generate(SimplePart *part);
	};


	/**
	 * Function generated.
	 */
	class ToSFunction : public Function {
		STORM_CLASS;
	public:
		// Create. Pass the 'toS(StrBuf)' to use.
		STORM_CTOR ToSFunction(Function *fn);

	protected:
		// Detect if the target type gets its own toS function.
		void STORM_FN notifyAdded(NameSet *to, Named *added);

	private:
		// Output function to use.
		Function *use;

		// Generate code.
		CodeGen *CODECALL create();
	};


	/**
	 * Provide a system-wide << function for value types that provide eiter toS() or toS(StrBuf).
	 */
	class OutputOperatorTemplate : public Template {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR OutputOperatorTemplate();

		virtual MAYBE(Named *) STORM_FN generate(SimplePart *part);
	};

	/**
	 * Output operator that is generated.
	 */
	class OutputOperator : public Function {
		STORM_CLASS;
	public:
		// Create. Pass the 'toS()' or 'toS(StrBuf)' to use.
		STORM_CTOR OutputOperator(Function *fn);

	private:
		// Function to use.
		Function *use;

		// Generate code.
		CodeGen *CODECALL create();
	};

}
