#include "stdafx.h"
#include "Node.h"
#include "Exception.h"
#include "Type.h"
#include "Engine.h"
#include "Package.h"
#include "Core/Str.h"

namespace storm {
	namespace syntax {

		Node::Node() {}

		Node::Node(SrcPos pos) : pos(pos) {}

		void Node::throwError() {
			throw new (this) SyntaxError(pos, S("Trying to transform a node not representing a match."));
		}

		Array<Node *> *Node::children() {
			return new (this) Array<Node *>();
		}

		Array<Node *> *Node::allChildren() {
			Array<Node *> *result = new (this) Array<Node *>();
			allChildren(result, null);
			return result;
		}

		Array<Node *> *Node::allChildren(Type *type) {
			Array<Node *> *result = new (this) Array<Node *>();
			allChildren(result, type);
			return result;
		}

		void Node::allChildren(Array<Node *> *result, MAYBE(Type *) type) {
			Array<Node *> *here = children();
			for (Nat i = 0; i < here->count(); i++) {
				Node *at = here->at(i);

				if (!type || runtime::typeOf(at)->isA(type))
					result->push(at);

				at->allChildren(result, type);
			}
		}


		const void *transformFunction(Type *type, const Value &result, const Value &param1, const Value &param2) {
			Scope root = type->engine.scope();
			Array<Value> *par = new (type) Array<Value>();
			par->push(thisPtr(type));
			if (param1 != Value())
				par->push(param1);
			if (param2 != Value())
				par->push(param2);
			SimplePart *part = new (type) SimplePart(new (type) Str(L"transform"), par);
			if (Function *fn = as<Function>(type->find(part, root))) {
				if (!result.mayReferTo(fn->result)) {
					Str *msg = TO_S(type, S("The function ") << fn->identifier() << S(" returns ")
									<< fn->result << S(", which is incompatible with ") << result);
					throw new (type) InternalError(msg);
				}

				return fn->ref().address();
			} else {
				Str *msg = TO_S(type, S("Can not find ") << part << S(" in ") << type->identifier());
				throw new (type) InternalError(msg);
			}
		}

	}
}
