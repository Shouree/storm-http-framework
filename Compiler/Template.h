#pragma once
#include "Name.h"
#include "Named.h"
#include "ValueArray.h"
#include "Core/Fn.h"
#include "Core/Gen/CppTypes.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Defines something that generated Named objects based on the parameter list given.
	 * An example of use-cases is to implement template types, but it could also be used
	 * to implement template functions and packages among other things.
	 *
	 * If this class is added to a NameSet, it will be queried if no regular Named is found
	 * and therefore has the chance to generate a Named that will be used as a substitute.
	 * Any created Named will be added to the NameSet as usual to ensure proper operation
	 * of the rest of the system.
	 */
	class Template : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Ctor.
		STORM_CTOR Template(Str *name);

		// Name.
		Str *name;

		// Called when something with our name is not found. Returns null if nothing is found.
		virtual MAYBE(Named *) STORM_FN generate(SimplePart *part);

		// Is this a placeholder template object? These will not be inserted in packages by 'TemplateList'.
		virtual Bool STORM_FN placeholder();
	};

	/**
	 * Template generated from a function in C++.
	 */
	class TemplateCppFn : public Template {
		STORM_CLASS;
	public:
		typedef CppTemplate::GenerateFn GenerateFn;

		// Ctor.
		TemplateCppFn(Str *name, GenerateFn fn);

		// Generate function.
		UNKNOWN(PTR_GC) GenerateFn fn;

		// Generate things.
		virtual MAYBE(Named *) STORM_FN generate(SimplePart *part);
		virtual MAYBE(Type *) STORM_FN generate(ValueArray *part);
	};


	/**
	 * Placeholder template for foreign template definitions from C++.
	 */
	class TemplatePlaceholder : public TemplateCppFn {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR TemplatePlaceholder(Str *name);

		// This is a placeholder!
		virtual Bool STORM_FN placeholder();

		// Generate things.
		virtual MAYBE(Named *) STORM_FN generate(SimplePart *part);
		virtual MAYBE(Type *) STORM_FN generate(ValueArray *part);
	};


	/**
	 * Template generated from a function in Storm/C++.
	 */
	class TemplateFn : public Template {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR TemplateFn(Str *name, Fn<MAYBE(Named *), Str *, SimplePart *> *fn);

		// Function for generation.
		Fn<MAYBE(Named *), Str *, SimplePart *> *fn;

		// Generate stuff.
		virtual MAYBE(Named *) STORM_FN generate(SimplePart *part);
	};

}
