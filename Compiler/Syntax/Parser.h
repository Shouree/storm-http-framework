#pragma once
#include "Compiler/NamedThread.h"
#include "Compiler/Exception.h"
#include "Core/Array.h"
#include "Node.h"
#include "InfoNode.h"
#include "Rule.h"
#include "Production.h"
#include "ParserBackend.h"

namespace storm {
	namespace syntax {
		STORM_PKG(core.lang);

#ifdef DEBUG
		extern bool parserDebug;
#endif

		// Default parser to use in the system. earley or glr
#define DEFAULT_PARSER glr

		/**
		 * Base class for the templated parser in Storm. In Storm, Parser<T> is to be
		 * instantiated. This is not possible from C++ as the syntax types are not present in
		 * C++. Therefore, the interface differs a bit (C++ loses some type safety compared to
		 * Storm).
		 *
		 * TODO: Make it possible to parse things on other threads than the compiler thread.
		 */
		class ParserBase : public ObjectOn<Compiler> {
			STORM_CLASS;
		protected:
			// Create. Create via Parser from C++ as the type has to be set properly for this to work.
			ParserBase(ParserBackend *backend);

			// Create an InfoParser.
			ParserBase(Rule *root, ParserBackend *backend);

			// Common parts from the constructors.
			void init(Rule *root, ParserBackend *backend);

			// Internal function called from Storm to create a parser.
			friend void createParser(void *mem, ParserBackend *backend);

		public:
			// Add a package containing syntax definitions (not recursive).
			void STORM_FN addSyntax(Package *pkg);
			void STORM_FN addSyntax(Array<Package *> *pkg);

			// Add a package contining syntax, possibly ignoring exports.
			void STORM_FN addSyntax(Package *pkg, Bool useExports);
			void STORM_FN addSyntax(Array<Package *> *pkg, Bool useExports);

			// Does this parser contain the same syntax as 'o'?
			Bool STORM_FN sameSyntax(ParserBase *o);

			// Get the root rule.
			virtual Rule *STORM_FN root() const;

			// Parse a string. Returns 'true' if we found some match.
			Bool STORM_FN parse(Str *str, Url *file);
			virtual Bool STORM_FN parse(Str *str, Url *file, Str::Iter start);

			// Clear all parse-related information. Included packages are retained.
			virtual void STORM_FN clear();

			/**
			 * Operations on the last parse.
			 */

			// Found any errors? If Str::Iter is not end, this is always true. Note that even if we
			// have an error, it could be possible to extract a tree!
			Bool STORM_FN hasError() const;

			// Is it possible to extract a syntax tree? (equivalent to the return value of 'parse').
			Bool STORM_FN hasTree() const;

			// Return an iterator after the last matched character, or the start of the string if no
			// match could be made.
			Str::Iter STORM_FN matchEnd() const;

			// Get the error (present if 'hasError' is true).
			SyntaxError *STORM_FN error() const;

			// Throw error if it is present.
			void STORM_FN throwError() const;

			// Get the error message.
			Str *STORM_FN errorMsg() const;

			// Get the syntax tree. Only for C++, in Storm we know the exact subtype we will generate!
			Node *tree() const;

			// Get the generic syntax tree.
			InfoNode *STORM_FN infoTree() const;

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;


			/**
			 * Performance inspection:
			 */

			// Get the number of states used.
			Nat STORM_FN stateCount() const;

			// Get the number of bytes used.
			Nat STORM_FN byteCount() const;

		protected:
			// Call 'parseApprox'. Only available from 'InfoParser'.
			inline InfoErrors parseApprox(Str *str, Url *file, Str::Iter start, MAYBE(InfoInternal *) context) {
				return use->parseApprox(root(), str, file, start, context);
			}

			// Get the raw info tree. Never throws an exception.
			inline MAYBE(InfoNode *) unsafeInfoTree() const {
				return use->infoTree();
			}

		private:
			// Backend being used.
			ParserBackend *use;

			// Private version for adding syntax.
			void addSyntaxI(Package *pkg);
		};


		/**
		 * Generic parser supporting only generating trees of InfoNodes. However, it is possible to
		 * change the root rule of this kind of parser.
		 */
		class InfoParser : public ParserBase {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR InfoParser(Rule *rootRule);
			STORM_CTOR InfoParser(Rule *rootRule, ParserBackend *backend);

			// Create with a package and a name.
			static InfoParser *create(Package *pkg, const wchar *name);
			static InfoParser *create(Package *pkg, const wchar *name, ParserBackend *backend);

			// Set a new root.
			void STORM_FN root(Rule *rule);

			// Get the root rule.
			virtual Rule *STORM_FN root() const;

			// Parse a string regularly. We need to keep track of what is being parsed.
			using ParserBase::parse;
			virtual Bool STORM_FN parse(Str *str, Url *file, Str::Iter start);

			// Parse a syntax tree using error recovery, optionally providing a context to properly
			// resolve dependencies when parsing parts of the input. If provided, the context is the
			// node that will be the parent of the node produced by this parse. Do not use 'tree'
			// after calling this, as 'tree' assumes a complete syntax tree (not possible from
			// Storm, as 'tree' is not exposed).  TODO: Support giving an explicit end as well.
			InfoErrors STORM_FN parseApprox(Str *str, Url *file);
			InfoErrors STORM_FN parseApprox(Str *str, Url *file, Str::Iter start);
			InfoErrors STORM_FN parseApprox(Str *str, Url *file, InfoInternal *context);
			InfoErrors STORM_FN parseApprox(Str *str, Url *file, Str::Iter start, InfoInternal *context);

			// Clear.
			virtual void STORM_FN clear();

			// Get an info tree that is always filled to the end of the tree, regardless of the
			// actual length of the match.
			InfoNode *STORM_FN fullInfoTree();

		private:
			// Root rule.
			Rule *rootRule;

			// Remember the last parsed string and start position.
			Str *lastStr;
			Nat lastOffset;
		};


		/**
		 * C++ instantiation, compatible enough to pass on to Storm. This class is not visible to
		 * the preprocessor, so use ParserBase for that.
		 *
		 * Make sure to not introduce any additional data members here, as they might get lost
		 * during accidental interfacing with Storm. This class should only contain functions that
		 * completes the interface from the C++ side.
		 */
		class Parser : public ParserBase {
		public:
			// Create a parser parsing a specific type.
			static Parser *create(Rule *root);
			static Parser *create(Rule *root, ParserBackend *backend);

			// Create a parser parsing the type named 'name' in 'pkg'.
			static Parser *create(Package *pkg, const wchar *name);
			static Parser *create(Package *pkg, const wchar *name, ParserBackend *backend);

			// Transform syntax nodes. See limitations for 'transformNode<>'.
			template <class R>
			R *transform() {
				return transformNode<R>(tree());
			}

			template <class R, class P>
			R *transform(P par) {
				return transformNode<R, P>(tree(), par);
			}

		private:
			// Use 'create'.
			Parser(ParserBackend *backend);
		};


		/**
		 * Create parser backends.
		 */
		Nat STORM_FN backendCount();
		ParserBackend *STORM_FN createBackend(EnginePtr e, Nat id);


		// Declare the template. This does not match the above declaration, so use ParserBase to
		// store Parser instances in classes!
		STORM_TEMPLATE(Parser, createParser);

	}
}
