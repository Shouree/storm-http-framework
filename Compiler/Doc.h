#pragma once
#include "Value.h"
#include "Visibility.h"
#include "Core/Str.h"
#include "Utils/Exception.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;

	/**
	 * Parameter names for use in documentation.
	 */
	class DocParam {
		STORM_VALUE;
	public:
		// Create.
		STORM_CTOR DocParam(Str *name, Value type);

		// Parameter name.
		Str *name;

		// Parameter type.
		Value type;

		// Output.
		void STORM_FN toS(StrBuf *to) const;
	};

	// To string.
	wostream &operator <<(wostream &to, DocParam p);


	/**
	 * Documentation note. Contains a type and a string that is to displayed alongside the name of
	 * the object. Used for things like return types and inheritance.
	 *
	 * A type of 'void' is ignored, which means that this class can be used to represent other kinds
	 * of information as well.
	 */
	class DocNote {
		STORM_VALUE;
	public:
		// Create.
		STORM_CTOR DocNote(Str *note, Value type);
		STORM_CTOR DocNote(Str *note, Named *named);
		STORM_CTOR DocNote(Str *note);

		// Note.
		Str *note;

		// Named object.
		MAYBE(Named *) named;

		// Reference?
		Bool ref;

		// Show the type at all?
		Bool showType;

		// Output.
		void STORM_FN toS(StrBuf *to) const;
	};

	// To string.
	wostream &operator <<(wostream &to, DocNote n);


	/**
	 * Documentation for a single named entity.
	 *
	 * Instances of this class is generally not stored in memory longer than necessary. Rather, each
	 * 'Named' instance may provide a 'NamedDoc' instance that allows retrieving 'Doc' instances for
	 * that entity when needed. Documentation is generally browsed by a user, and as such it is
	 * desirable to load documentation on demand (since we can be fast enough) rather than preemptively.
	 *
	 * TODO: Include a reference to the Named or Template we are referring to?
	 * TODO: Include source position here or in the Named itself.
	 * TODO: Include a value that allows sorting the documentation for a type in some useful order.
	 */
	class Doc : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Doc(Str *name, Array<DocParam> *params, MAYBE(Visibility *) v, Str *body);
		STORM_CTOR Doc(Str *name, Array<DocParam> *params, DocNote note, MAYBE(Visibility *) v, Str *body);
		STORM_CTOR Doc(Str *name, Array<DocParam> *params, Array<DocNote> *notes, MAYBE(Visibility *) v, Str *body);

		// Name of this entity.
		Str *name;

		// Parameters.
		Array<DocParam> *params;

		// Notes.
		Array<DocNote> *notes;

		// Visibility of this entity.
		MAYBE(Visibility *) visibility;

		// Documentation body. Formatted using a format similar to Markdown.
		Str *body;

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Generate a one-line summary of the documentation.
		Str *STORM_FN summary() const;

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

	// Create documentation, infer 'name' and 'notes' from an entity.
	Doc *STORM_FN doc(Named *entity, Array<DocParam> *params, Str *body) ON(Compiler);

	// Create a dummy documentation block for 'entity'. Used when no NamedDoc is provided.
	Doc *STORM_FN doc(Named *entity) ON(Compiler);


	/**
	 * Documentation provider for named entities.
	 *
	 * TODO: It would be interesting to be able to query for a 'minimal' documentation object that
	 * only contains enough information to make the 'summary' interesting. This would mean only
	 * loading the parameter names and any notes essentially.
	 */
	class NamedDoc : public ObjectOn<Compiler> {
		STORM_ABSTRACT_CLASS;
	public:
		// Create.
		STORM_CTOR NamedDoc();

		// Get documentation for this named entity.
		virtual Doc *STORM_FN get() ABSTRACT;
	};


	/**
	 * Exception for documentation errors.
	 */
	class EXCEPTION_EXPORT DocError : public Exception {
		STORM_EXCEPTION;
	public:
		DocError(const wchar *msg) {
			w = new (this) Str(msg);
		}
		STORM_CTOR DocError(Str *msg) {
			w = msg;
		}
		virtual void STORM_FN message(StrBuf *to) const {
			*to << w;
		}
	private:
		Str *w;
	};


}
