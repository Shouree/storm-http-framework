#pragma once
#include "Core/StrBuf.h"
#include "Compiler/Name.h"
#include "Compiler/Package.h"
#include "RepType.h"
#include "InfoIndent.h"
#include "TokenColor.h"
#include "Delimiters.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		class FileContents;
		class Token;
		class Rule;
		class ProductionType;

		/**
		 * Logic for storing a parsed version of a syntax file before it is transformed into the
		 * real representation (which requires type resolution).
		 */

		/**
		 * An item in a syntax file.
		 */
		class FileItem : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR FileItem();
		};


		/**
		 * Use declaration.
		 */
		class UseDecl : public FileItem {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR UseDecl(SrcName *pkg);

			// Package to use.
			SrcName *pkg;

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Delimiter declaration.
		 */
		class DelimDecl : public FileItem {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR DelimDecl(SrcName *token, delim::Delimiter type);

			// Delimiter to use.
			SrcName *token;

			// Type of delimiter.
			delim::Delimiter type;

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};

		// Convenience creation from the grammar.
		DelimDecl *STORM_FN optionalDecl(SrcName *token);
		DelimDecl *STORM_FN requiredDecl(SrcName *token);
		DelimDecl *STORM_FN allDecl(SrcName *token);


		/**
		 * Parameter declaration.
		 */
		class ParamDecl {
			STORM_VALUE;
		public:
			STORM_CTOR ParamDecl(Name *type, Str *name);

			Name *type;
			Str *name;

			void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Token in a production declaration.
		 */
		class TokenDecl : public Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR TokenDecl();

			// Store this token as 'store'.
			MAYBE(Str *) store;

			// Use this token to invoke 'invoke'.
			MAYBE(Str *) invoke;

			// Capture the raw syntax tree?
			Bool raw;

			// Color of this token.
			TokenColor color;

			// Mark this token as 'raw'.
			void STORM_FN pushRaw(Str *dummy);

			// Mark this token as 'store as X' or 'invoke X'.
			void STORM_FN pushStore(Str *store);
			void STORM_FN pushInvoke(Str *invoke);

			// Add a color to this token.
			void STORM_FN pushColor(SStr *color);

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Is this a delimiter?
			virtual Bool STORM_FN delimiter() const { return false; }

			// Create this token.
			virtual Token *create(SrcPos pos, Scope scope, Delimiters *delimiters);
		};


		// Unescape string literals.
		Str *STORM_FN unescapeStr(Str *s);


		/**
		 * Regex token declaration.
		 */
		class RegexTokenDecl : public TokenDecl {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR RegexTokenDecl(Str *regex);

			// Regex.
			Str *regex;

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Create this token.
			virtual Token *create(SrcPos pos, Scope scope, Delimiters *delimiters);
		};


		/**
		 * Token matching a rule.
		 */
		class RuleTokenDecl : public TokenDecl {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR RuleTokenDecl(SrcPos pos, Name *rule);
			STORM_CTOR RuleTokenDecl(SrcPos pos, Name *rule, Array<Str *> *params);

			// Where?
			SrcPos pos;

			// Name to match.
			Name *rule;

			// Parameters to this token. null means no parens were given.
			MAYBE(Array<Str *> *) params;

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Create this token.
			virtual Token *create(SrcPos pos, Scope scope, Delimiters *delimiters);
		};


		/**
		 * Token matching a delimiter (i.e. ',' or '~').
		 */
		class DelimTokenDecl : public TokenDecl {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR DelimTokenDecl(delim::Delimiter type);

			// Type of delimiter.
			delim::Delimiter type;

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Is this a delimiter?
			virtual Bool STORM_FN delimiter() const { return true; }

			// Create this token.
			virtual Token *create(SrcPos pos, Scope scope, Delimiters *delimiters);
		};

		// Create from the grammar.
		DelimTokenDecl *STORM_FN optionalTokenDecl(EnginePtr e);
		DelimTokenDecl *STORM_FN requiredTokenDecl(EnginePtr e);

		/**
		 * Dummy token representing a - separator.
		 */
		class SepTokenDecl : public TokenDecl {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR SepTokenDecl();

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Is this a delimiter?
			virtual Bool STORM_FN delimiter() const { return true; }

			// Create this token.
			virtual Token *create(SrcPos pos, Scope scope, Delimiters *delimiters);
		};


		/**
		 * Representation of a declared production.
		 */
		class ProductionDecl : public FileItem {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR ProductionDecl(SrcPos pos, Name *memberOf);
			STORM_CTOR ProductionDecl(SrcPos pos, Name *memberOf, MAYBE(Name *) parent);

			// Where was this production declared?
			SrcPos pos;

			// Location of the documentation (if any).
			SrcPos docPos;

			// Do we require any parent?
			MAYBE(Name *) parent;

			// Which rule are we a member of?
			Name *rule;

			// Priority.
			Int priority;

			// Push priority.
			void STORM_FN pushPrio(Int priority);

			// Tokens.
			Array<TokenDecl *> *tokens;

			// Push a token. Ignores SepTokenDecl.
			void STORM_FN push(TokenDecl *token);

			// Any specific name of this rule?
			MAYBE(Str *) name;

			// Set the name.
			void STORM_FN pushName(Str *name);

			// Result (if given).
			MAYBE(Name *) result;

			// Set the result.
			void STORM_FN pushResult(Name *result);
			void STORM_FN pushResult(Str *result);

			// Parameters to the result. null = no parens present.
			MAYBE(Array<Str *> *) resultParams;

			// Set result parameters.
			void STORM_FN pushParams(Array<Str *> *params);

			// Repetition.
			Nat repStart;
			Nat repEnd;
			RepType repType;

			// Capture the repeat? Only supported if 'repType' is 'repNone'.
			MAYBE(TokenDecl *) repCapture;

			// Push repetition start and end.
			void STORM_FN pushRepStart(Str *dummy);
			void STORM_FN pushRepEnd(RepType type);
			void STORM_FN pushRepEnd(TokenDecl *capture);

			// Indentation.
			Nat indentStart;
			Nat indentEnd;
			IndentType indentType;

			// Push indentation start and end.
			void STORM_FN pushIndentStart(Str *dummy);
			void STORM_FN pushIndentEnd(IndentType type);

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Create.
			virtual ProductionType *STORM_FN create(Package *into, Delimiters *delimiters, Scope scope);

		private:
			// Output the end of a repetition.
			void outputRepEnd(StrBuf *to) const;
		};


		/**
		 * Rule declaration.
		 */
		class RuleDecl : public FileItem {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR RuleDecl(SrcPos pos, Str *name, Name *result);

			// Declared at?
			SrcPos pos;

			// Documentation location, if any?
			SrcPos docPos;

			// Name.
			Str *name;

			// Result type.
			Name *result;

			// Parameters.
			Array<ParamDecl> *params;

			// Default color when this rule is used as a token.
			TokenColor color;

			// Set parameters.
			void STORM_FN push(Array<ParamDecl> *params);

			// Set color.
			void STORM_FN pushColor(SStr *color);

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Create.
			virtual Rule *STORM_FN create(Scope scope);
		};


		/**
		 * Custom declaration. Expected to expand into multiple other declarations when added to a
		 * 'FileContents' object.
		 */
		class CustomDecl : public FileItem {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR CustomDecl();

			// Expand into other declarations. Add to 'to'.
			virtual void STORM_FN expand(FileContents *to);
		};


		/**
		 * Contents of an entire file.
		 */
		class FileContents : public Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR FileContents();

			// Used packages.
			Array<SrcName *> *use;

			// Name of the optional delimiter (if any).
			MAYBE(SrcName *) optionalDelimiter;

			// Name of the required delimiter (if any).
			MAYBE(SrcName *) requiredDelimiter;

			// Rule declarations.
			Array<RuleDecl *> *rules;

			// Productions.
			Array<ProductionDecl *> *productions;

			// Add an item to the correct array.
			virtual void STORM_FN push(FileItem *item);

			// Get delimiter object.
			Delimiters *STORM_FN delimiters(Scope scope);

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Push a delimiter.
			void pushDelimiter(DelimDecl *decl);

			// Resolve a single delimiter.
			Rule *resolveDelimiter(Scope scope, SrcName *name);
		};

		// Join a set of strings into a dot-separated name. Used in the grammar.
		Str *STORM_FN joinName(Str *first, Array<Str *> *rest);

		// Attach documentation to a rule or a production.
		RuleDecl *STORM_FN applyDoc(SrcPos doc, RuleDecl *decl);
		ProductionDecl *STORM_FN applyDoc(SrcPos doc, ProductionDecl *decl);
		FileItem *STORM_FN applyDoc(SrcPos doc, FileItem *decl);

	}
}
