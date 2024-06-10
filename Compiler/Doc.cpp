#include "stdafx.h"
#include "Doc.h"
#include "Named.h"
#include "Exception.h"
#include "Function.h"
#include "Variable.h"
#include "Type.h"
#include "Package.h"
#include "Core/Join.h"

namespace storm {

	/**
	 * DocParam.
	 */

	DocParam::DocParam(Str *name, Value type) : name(name), type(type) {}

	void DocParam::toS(StrBuf *to) const {
		if (name->any())
			*to << type << S(" ") << name;
		else
			*to << type;
	}

	wostream &operator <<(wostream &to, DocParam p) {
		if (p.name->any())
			return to << p.type << L" " << p.name;
		else
			return to << p.type;
	}


	/**
	 * DocNote.
	 */

	DocNote::DocNote(Str *note, Value type) : note(note), named(type.type), ref(type.ref), showType(true) {}

	DocNote::DocNote(Str *note, Named *named) : note(note), named(named), ref(false), showType(true) {}

	DocNote::DocNote(Str *note) : note(note), named(null), ref(false), showType(false) {}

	void DocNote::toS(StrBuf *to) const {
		*to << note;
		if (showType) {
			*to << S(" ");
			if (named) {
				*to << named->identifier();
				if (ref)
					*to << S("&");
			} else {
				*to << S("void");
			}
		}
	}

	wostream &operator <<(wostream &to, DocNote n) {
		to << n.note;
		if (n.showType) {
			to << L" ";
			if (n.named) {
				to << n.named->identifier();
				if (n.ref)
					to << L"&";
			} else {
				to << L"void";
			}
		}
		return to;
	}

	static DocNote note(Engine &e, const wchar *note, Value type) {
		return DocNote(new (e) Str(note), type);
	}

	static DocNote note(Engine &e, const wchar *note, Named *named) {
		return DocNote(new (e) Str(note), named);
	}

	static DocNote note(Engine &e, const wchar *note) {
		return DocNote(new (e) Str(note));
	}


	/**
	 * Doc.
	 */

	Doc::Doc(Str *name, Array<DocParam> *params, MAYBE(Visibility *) v, Str *body) :
		name(name), params(params), notes(new (engine()) Array<DocNote>()), visibility(v), body(body) {}

	Doc::Doc(Str *name, Array<DocParam> *params, DocNote note, MAYBE(Visibility *) v, Str *body) :
		name(name), params(params), notes(new (engine()) Array<DocNote>(1, note)), visibility(v), body(body) {}

	Doc::Doc(Str *name, Array<DocParam> *params, Array<DocNote> *notes, MAYBE(Visibility *) v, Str *body) :
		name(name), params(params), notes(notes), visibility(v), body(body) {}

	void Doc::deepCopy(CloneEnv *env) {
		cloned(name, env);
		cloned(params, env);
		cloned(notes, env);
		cloned(body, env);
	}

	static void putSummary(StrBuf *to, const Doc *doc) {
		*to << doc->name;
		if (doc->params->any())
			*to << S("(") << join(doc->params, S(", ")) << S(")");
		if (doc->notes->any())
			*to << S(" ") << join(doc->notes, S(", "));
	}

	Str *Doc::summary() const {
		StrBuf *to = new (this) StrBuf();
		putSummary(to, this);
		if (visibility)
			*to << S(" [") << visibility << S("]");
		return to->toS();
	}

	void Doc::toS(StrBuf *to) const {
		if (visibility)
			*to << visibility << S(" ");
		putSummary(to, this);
		*to << S(":\n\n");
		*to << body;
	}

	static void addFlags(FnFlags fn, Array<DocNote> *notes) {
		Engine &e = notes->engine();
		FnFlags flags[] = { fnPure, fnFinal, fnAbstract, fnOverride, fnStatic };
		const wchar *names[] = { S("pure"), S("final"), S("abstract"), S("override"), S("static") };

		for (Nat i = 0; i < ARRAY_COUNT(flags); i++) {
			if ((fn & flags[i]) == flags[i])
				notes->push(note(e, names[i]));
		}
	}

	static Array<DocNote> *createNotes(Named *entity) {
		Engine &e = entity->engine();
		Array<DocNote> *notes = new (e) Array<DocNote>();

		if (Function *fn = as<Function>(entity)) {
			notes->push(note(e, S("->"), fn->result));
			RunOn on = fn->runOn();
			if (on.state != RunOn::any)
				notes->push(note(e, S("on"), on.thread));
			if (fn->fnFlags() & fnAssign)
				notes->push(note(e, S("assign")));

			addFlags(fn->fnFlags(), notes);
		} else if (Variable *var = as<Variable>(entity)) {
			notes->push(note(e, S("->"), var->type));
			if (GlobalVar *global = as<GlobalVar>(var))
				notes->push(note(e, S("on"), global->owner));
		} else if (Type *type = as<Type>(entity)) {
			if (Type *super = type->super())
				notes->push(note(e, S("extends"), Value(super)));
			RunOn on = type->runOn();
			if (on.state != RunOn::any)
				notes->push(note(e, S("on"), on.thread));
		} else if (as<Package>(entity)) {
			notes->push(note(e, S("(package)")));
		}

		return notes;
	}

	Doc *doc(Named *entity, Array<DocParam> *params, Str *body) {
		return new (entity) Doc(entity->name, params, createNotes(entity), entity->visibility, body);
	}

	Doc *doc(Named *entity) {
		Array<DocParam> *params = new (entity) Array<DocParam>();
		for (Nat i = 0; i < entity->params->count(); i++)
			params->push(DocParam(new (entity) Str(S("")), entity->params->at(i)));

		return new (entity) Doc(entity->name, params, createNotes(entity), entity->visibility, new (entity) Str(S("")));
	}


	/**
	 * NamedDoc.
	 */

	NamedDoc::NamedDoc() {}

}
