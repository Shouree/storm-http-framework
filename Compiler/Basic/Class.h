#pragma once
#include "Core/Map.h"
#include "Core/Array.h"
#include "Compiler/Type.h"
#include "Compiler/Syntax/SStr.h"
#include "Compiler/Syntax/Node.h"
#include "Compiler/Scope.h"
#include "Compiler/Visibility.h"
#include "Param.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class BSRawFn;
		class BSFunction;
		class BSCtor;

		class Class : public Type {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Class(TypeFlags flags, SrcPos pos, Scope scope, Str *name, syntax::Node *body);

			// The scope used for this class.
			Scope scope;

			// Lookup any additional types needed.
			void lookupTypes();

			// Set the super class from a decorator.
			void STORM_FN super(SrcName *super);
			using Type::super;

			// Set the current thread from a decorator.
			void STORM_FN thread(SrcName *thread);

			// Set the current thread to "unknown" from a decorator.
			void STORM_FN unknownThread(SrcPos pos);

			// Set the thread to use by default if no other thread is specified.
			void STORM_FN defaultThread(SrcName *thread);

			// Add a decorator.
			void STORM_FN decorate(SrcName *decorator);

			// Make the class final.
			void STORM_FN makeFinal();

		protected:
			// Load the body lazily.
			virtual Bool STORM_FN loadAll();

		private:
			// Body of the class.
			MAYBE(syntax::Node *) body;

			// Decorators used.
			MAYBE(Array<SrcName *> *) decorators;

			// Name for either the parent class, or the named thread we shall use. Depending on 'nameIsThread'.
			MAYBE(SrcName *) otherName;

			// What does 'otherName' mean currently?
			Byte otherMeaning;

			enum {
				// Nothing yet.
				otherNone,

				// Default thread to use.
				otherDefaultThread,

				// Thread to use (non-default, should not be set again). 'otherName' may be null.
				otherThread,

				// Super class to use.
				otherSuper,
			};

			// Allowing lazy loads?
			Bool allowLazyLoad;

			// State used to keep track of what is added.
			struct AddState {
				AddState();

				bool ctor;
				bool copyCtor;
				bool deepCopy;
				bool assign;
			};

			// Add a member to the class, keeping track of what already exists.
			void addMember(Named *member, AddState &s);
		};


		// Create a class or a value.
		Class *STORM_FN createClass(SrcPos pos, Scope env, syntax::SStr *name, syntax::Node *body);
		Class *STORM_FN createValue(SrcPos pos, Scope env, syntax::SStr *name, syntax::Node *body);


		/**
		 * Wrapper around syntax nodes for class members so that we can attach a visibilty to them.
		 */
		class MemberWrap : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR MemberWrap(syntax::Node *node);

			// Node we're wrapping.
			syntax::Node *node;

			// Visibility of this node (if attached).
			MAYBE(Visibility *) visibility;

			// If set: location of the documentation for this member.
			SrcPos docPos;

			// Transform this node.
			virtual Named *STORM_FN transform(Class *owner);
		};


		/**
		 * Class body.
		 */
		class ClassBody : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR ClassBody(Class *owner);

			// Add content.
			virtual void STORM_FN add(Named *item);

			// Add a wrapped syntax node.
			virtual void STORM_FN add(MemberWrap *wrap);

			// Add template.
			virtual void STORM_FN add(Template *t);

			// Add default access modifier.
			virtual void STORM_FN add(Visibility *v);

			// Add any of the above things.
			virtual void STORM_FN add(TObject *t);

			// Called before the class attempts to extract data from the class. Allows extensions to
			// get a last chance at altering the data before we process it.
			virtual void STORM_FN prepareItems();

			// Called after 'items' are loaded but before 'wraps' are loaded.
			virtual void STORM_FN prepareWraps();

			// Owning class.
			Class *owner;

			// Contents.
			Array<Named *> *items;

			// Wrapped members for later evaluation.
			Array<MemberWrap *> *wraps;

			// Template contents.
			Array<Template *> *templates;

			// Currently default access modifier.
			Visibility *defaultVisibility;
		};


		/**
		 * Class variable.
		 */
		MemberVar *STORM_FN classVar(Class *owner, SrcName *type, syntax::SStr *name) ON(Compiler);
		MemberVar *STORM_FN classVar(Class *owner, SrcName *type, syntax::SStr *name, MAYBE(syntax::Node *) init) ON(Compiler);

		/**
		 * Class function.
		 */
		BSFunction *STORM_FN classFn(Class *owner,
									SrcPos pos,
									syntax::SStr *name,
									Name *result,
									Array<NameParam> *params,
									syntax::Node *content) ON(Compiler);

		/**
		 * Abstract function in a class.
		 */
		Function *STORM_FN classAbstractFn(Class *owner,
										SrcPos pos,
										syntax::SStr *name,
										Name *result,
										Array<NameParam> *params,
										syntax::Node *options) ON(Compiler);

		/**
		 * Class function declared as 'assign function'.
		 */
		BSFunction *STORM_FN classAssign(Class *owner,
										SrcPos pos,
										syntax::SStr *name,
										Array<NameParam> *params,
										syntax::Node *content) ON(Compiler);

		/**
		 * Class constructor.
		 */
		BSCtor *STORM_FN classCtor(Class *owner,
								SrcPos pos,
								Array<NameParam> *params,
								syntax::Node *content) ON(Compiler);

		BSCtor *STORM_FN classCastCtor(Class *owner,
									SrcPos pos,
									Array<NameParam> *params,
									syntax::Node *content) ON(Compiler);

		BSCtor *STORM_FN classDefaultCtor(Class *owner) ON(Compiler);


	}
}
