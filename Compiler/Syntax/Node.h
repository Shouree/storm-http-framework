#pragma once
#include "Compiler/NamedThread.h"
#include "Core/SrcPos.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Base class for all syntax nodes. This class will be overridden by 'Rule' to add the
		 * 'transform' function and then by 'Production' to implement the 'transform' function for that
		 * specific option.
		 */
		class Node : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR Node();
			STORM_CTOR Node(SrcPos pos);

			// Start position for this node.
			SrcPos pos;

			// Throw an exception. Used when calling 'transform' from a pure Rule object.
			void CODECALL throwError();

			/**
			 * Introspection:
			 */

			// Get all direct children in this node as an array.
			virtual Array<Node *> *STORM_FN children();

			// Get all children (both direct and indirect) as a flat array.
			Array<Node *> *STORM_FN allChildren();

			// Get all children of a specific type as a plain array.
			Array<Node *> *STORM_FN allChildren(Type *type);

		private:
			// Internal helper for 'allChildren'.
			void allChildren(Array<Node *> *to, MAYBE(Type *) type);
		};


		/**
		 * Transform syntax nodes in C++. Assumes the result is a pointer.
		 */

		// Find the appropriate 'transform' function in a specific type. If 'param' is void, no
		// params are assumed except the this pointer.
		const void *transformFunction(Type *type, const Value &result, const Value &param1, const Value &param2);

		template <class R>
		R *transformNode(Node *node) {
			Engine &e = node->engine();
			const void *fn = transformFunction(runtime::typeOf(node), Value(StormInfo<R>::type(e)), Value(), Value());

			os::FnCall<R *> p = os::fnCall().add(node);
			return p.call(fn, true);
		}


		template <class R, class P>
		R *transformNode(Node *node, P par) {
			Engine &e = node->engine();
			const void *fn = transformFunction(runtime::typeOf(node),
											Value(StormInfo<R>::type(e)),
											Value(StormInfo<P>::type(e)),
											Value());

			os::FnCall<R *> p = os::fnCall().add(node).add(par);
			return p.call(fn, true);
		}

		template <class R, class P, class Q>
		R *transformNode(Node *node, P par1, Q par2) {
			Engine &e = node->engine();
			const void *fn = transformFunction(runtime::typeOf(node),
											Value(StormInfo<R>::type(e)),
											Value(StormInfo<P>::type(e)),
											Value(StormInfo<Q>::type(e)));

			os::FnCall<R *> p = os::fnCall().add(node).add(par1).add(par2);
			return p.call(fn, true);
		}

	}
}
