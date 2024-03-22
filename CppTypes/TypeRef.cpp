#include "stdafx.h"
#include "TypeRef.h"
#include "Exception.h"
#include "World.h"

TypeRef::TypeRef(const SrcPos &pos) : pos(pos), constType(false) {}


TemplateType::TemplateType(const SrcPos &pos, const CppName &name, const vector<Auto<TypeRef>> &params) :
	TypeRef(pos), name(name), params(params) {}

Size TemplateType::size() const {
	throw Error(L"Templates are unsupported in general!", pos);
}

Auto<TypeRef> TemplateType::resolve(World &in, const CppName &context) const {
	vector<Auto<TypeRef>> p = params;
	for (nat i = 0; i < p.size(); i++)
		p[i] = p[i]->resolve(in, context);

	// This is special, as it is not exported to Storm, but we have to know about it to properly GC it.
	if (name == L"GcArray" && p.size() == 1) {
		return new GcArrayType(pos, p[0], false);
	} else if (name == L"GcWeakArray" && p.size() == 1) {
		return new GcArrayType(pos, p[0], true);
	} else if (Template *found = in.templates.findUnsafe(name, context)) {
		// Found it! Note that the Maybe<> type is special.
		if (found->name == L"storm::Maybe") {
			return new MaybeValueType(p[0]);
		} else {
			return new ResolvedTemplateType(pos, found, p);
		}
	} else {
		// Nothing found. Remain a unique, non-gc:d type.
		return new TemplateType(pos, name, params);
	}
}

void TemplateType::print(wostream &to) const {
	to << name << L"<";
	join(to, params, L", ");
	to << L">";
}

size_t TemplateType::sortId() const {
	if (params.empty())
		return 0;
	else
		return params[0]->sortId();
}

ResolvedTemplateType::ResolvedTemplateType(const SrcPos &pos, Template *templ, const vector<Auto<TypeRef>> &params) :
	TypeRef(pos), type(templ), params(params) {}

Size ResolvedTemplateType::size() const {
	throw Error(L"Can not use templates as values, as their size is unknown.", pos);
}

Auto<TypeRef> ResolvedTemplateType::resolve(World &in, const CppName &context) const {
	vector<Auto<TypeRef>> p = params;
	for (nat i = 0; i < p.size(); i++)
		p[i] = p[i]->resolve(in, context);
	return new ResolvedTemplateType(pos, type, p);
}

void ResolvedTemplateType::print(wostream &to) const {
	to << type->name << L"<";
	join(to, params, L", ");
	to << L">";
}

size_t ResolvedTemplateType::sortId() const {
	if (params.empty())
		return type->id;
	else
		return params[0]->sortId() + type->id + 1;
}

PtrType::PtrType(Auto<TypeRef> of) : TypeRef(of->pos), of(of) {}

Auto<TypeRef> PtrType::resolve(World &in, const CppName &context) const {
	Auto<PtrType> r = new PtrType(*this);
	r->of = of->resolve(in, context);
	return r;
}

void PtrType::print(wostream &to) const {
	to << of << L"*";
}

size_t PtrType::sortId() const {
	return of->sortId();
}

RefType::RefType(Auto<TypeRef> of) : TypeRef(of->pos), of(of) {}

Auto<TypeRef> RefType::resolve(World &in, const CppName &context) const {
	Auto<RefType> r = new RefType(*this);
	r->of = of->resolve(in, context);
	return r;
}

void RefType::print(wostream &to) const {
	to << of << L"&";
}

size_t RefType::sortId() const {
	return of->sortId();
}

MaybeClassType::MaybeClassType(Auto<TypeRef> of) : TypeRef(of->pos) {
	if (this->of = of.as<PtrType>())
		;
	else
		throw Error(L"MAYBE(T) must contain a pointer, use Maybe<T> instead.", of->pos);
}

Auto<TypeRef> MaybeClassType::resolve(World &in, const CppName &context) const {
	Auto<MaybeClassType> r = new MaybeClassType(*this);
	r->of = of->resolve(in, context).as<PtrType>();
	if (!r->of)
		throw Error(L"MAYBE content turned into non-pointer.", of->pos);
	return r;
}

void MaybeClassType::print(wostream &to) const {
	to << L"MAYBE(" << of << L")";
}

size_t MaybeClassType::sortId() const {
	return of->sortId();
}

MaybeValueType::MaybeValueType(Auto<TypeRef> of) : TypeRef(of->pos), of(of) {
	if (of.as<PtrType>())
		throw Error(L"Maybe<T> must refer to a value, use MAYBE(T *) instead.", of->pos);
}

Size MaybeValueType::size() const {
	Size s = of->size();
	// Align to struct boundary (should already be done).
	s += s.align();

	// Add a bool and align again.
	s += Size::sByte;
	s += s.align();

	return s;
}

Auto<TypeRef> MaybeValueType::resolve(World &in, const CppName &context) const {
	Auto<MaybeValueType> r = new MaybeValueType(*this);
	r->of = of->resolve(in, context);
	if (Auto<ResolvedType> x = r->of.as<ResolvedType>()) {
		if (Class *c = as<Class>(x->type)) {
			if (c->hasDtor())
				throw Error(L"The maybe-type does not support values with destructors or copy-ctors.", of->pos);
		}
	}
	return r;
}

void MaybeValueType::print(wostream &to) const {
	to << L"storm::Maybe<" << of << L">";
}

size_t MaybeValueType::sortId() const {
	return of->sortId();
}

NamedType::NamedType(const SrcPos &pos, const CppName &name) : TypeRef(pos), name(name) {}

NamedType::NamedType(const SrcPos &pos, const String &name) : TypeRef(pos), name(name) {}

Size NamedType::size() const {
	throw Error(L"Unknown size of the non-exported type " + name +
				L"\nUse PTR_NOGC or similar if this is the intent.", pos);
}

Auto<TypeRef> NamedType::resolve(World &in, const CppName &context) const {
	Type *t = in.types.findUnsafe(name, context);
	if (t)
		return new ResolvedType(*this, t);
	else if (name == L"void")
		return new VoidType(pos);
	else if (name == L"GcWatch" || name == L"storm::GcWatch")
		return new GcWatchType(pos);
	else if (name == L"EnginePtr" || name == L"storm::EnginePtr")
		return new EnginePtrType(pos);
	else if (name == L"GcType" || name == L"storm::GcType")
		return new GcTypeType(pos);

	// Last restort: built in type?
	map<String, Size>::const_iterator i = in.builtIn.find(name);
	if (i != in.builtIn.end())
		return new BuiltInType(pos, name, i->second);
	else
		return new NamedType(*this);
}

void NamedType::print(wostream &to) const {
	to << name;
}

size_t NamedType::sortId() const {
	return 0;
}


ResolvedType::ResolvedType(const TypeRef &templ, Type *type) : TypeRef(templ), type(type) {}

ResolvedType::ResolvedType(Type *type) : TypeRef(type->pos), type(type) {}

Size ResolvedType::size() const {
	return type->size();
}

bool ResolvedType::gcType() const {
	return type->heapAlloc();
}

Auto<TypeRef> ResolvedType::resolve(World &in, const CppName &context) const {
	return new ResolvedType(*this, type);
}

void ResolvedType::print(wostream &to) const {
	to << type->name;
}

size_t ResolvedType::sortId() const {
	return (type->id + 1) * 100;
}


BuiltInType::BuiltInType(const SrcPos &pos, const String &name, Size size) : TypeRef(pos), name(name), tSize(size) {}

Auto<TypeRef> BuiltInType::resolve(World &in, const CppName &context) const {
	return new BuiltInType(pos, name, tSize);
}

void BuiltInType::print(wostream &to) const {
	to << name;
}

size_t BuiltInType::sortId() const {
	return 0;
}


VoidType::VoidType(const SrcPos &pos) : TypeRef(pos) {}

Auto<TypeRef> VoidType::resolve(World &in, const CppName &context) const {
	return new VoidType(pos);
}

void VoidType::print(wostream &to) const {
	to << L"void";
}

size_t VoidType::sortId() const {
	return 0;
}


GcArrayType::GcArrayType(const SrcPos &pos, Auto<TypeRef> of, bool weak) : TypeRef(pos), of(of), weak(weak) {}

Auto<TypeRef> GcArrayType::resolve(World &in, const CppName &context) const {
	Auto<GcArrayType> r = new GcArrayType(*this);
	r->of = of->resolve(in, context);
	return r;
}

Size GcArrayType::size() const {
	throw Error(L"GcArray<> should only be used as a pointer!", pos);
}

void GcArrayType::print(wostream &to) const {
	if (weak)
		to << L"storm::GcWeakArray<" << of << L">";
	else
		to << L"storm::GcArray<" << of << L">";
}

size_t GcArrayType::sortId() const {
	return of->sortId();
}


GcWatchType::GcWatchType(const SrcPos &pos) : TypeRef(pos) {}

Auto<TypeRef> GcWatchType::resolve(World &in, const CppName &context) const {
	return new GcWatchType(*this);
}

Size GcWatchType::size() const {
	throw Error(L"GcWatch should only be used as a pointer!", pos);
}

void GcWatchType::print(wostream &to) const {
	to << L"storm::GcWatch";
}

size_t GcWatchType::sortId() const {
	return 0;
}


Auto<TypeRef> EnginePtrType::resolve(World &in, const CppName &context) const {
	return new EnginePtrType(*this);
}

Size EnginePtrType::size() const {
	throw Error(L"EnginePtr should only be used as parameters to functions.", pos);
}

void EnginePtrType::print(wostream &to) const {
	to << L"storm::EnginePtr";
}

size_t EnginePtrType::sortId() const {
	return 0;
}

Auto<TypeRef> GcTypeType::resolve(World &in, const CppName &context) const {
	return new GcTypeType(*this);
}

Size GcTypeType::size() const {
	throw Error(L"GcType should only be used as a pointer.", pos);
}

void GcTypeType::print(wostream &to) const {
	to << L"storm::GcType";
}

size_t GcTypeType::sortId() const {
	return 0;
}


const UnknownType::ID UnknownType::ids[] = {
	{ L"PTR_NOGC", Size::sPtr, false },
	{ L"PTR_GC", Size::sPtr, true },
	{ L"INT", Size::sInt, false },
	{ null, Size::sInt, false },
};

UnknownType::UnknownType(const CppName &kind, Auto<TypeRef> of) : TypeRef(of->pos), of(of), id(null) {
	for (nat i = 0; ids[i].name; i++) {
		if (kind == ids[i].name) {
			id = &ids[i];
			break;
		}
	}

	if (!id)
		alias = new NamedType(of->pos, kind);
}

Size UnknownType::size() const {
	if (id)
		return id->size;
	else
		return alias->size();
}

bool UnknownType::gcType() const {
	if (id)
		return id->gc;
	else
		return alias->gcType();
}

Auto<TypeRef> UnknownType::resolve(World &in, const CppName &ctx) const {
	if (id) {
		return new UnknownType(*this);
	} else if (alias) {
		Auto<TypeRef> r = alias->resolve(in, ctx);
		UnknownType *u = new UnknownType(*this);
		u->alias = r;
		return u;
	} else {
		throw Error(L"Invalid UNKNOWN() declaration.", of->pos);
	}
}

Auto<TypeRef> UnknownType::wrapper(World &in) const {
	if (!id)
		return alias;
	else
		return new ResolvedType(*this, in.unknown(id->name, pos));
}

void UnknownType::print(wostream &to) const {
	if (id) {
		to << L"UNKNOWN(" << id->name << L") " << of;
	} else {
		to << L"UNKNOWN(" << alias << L") " << of;
	}
}

size_t UnknownType::sortId() const {
	return alias->sortId();
}
