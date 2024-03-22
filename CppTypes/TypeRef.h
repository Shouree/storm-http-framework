#pragma once
#include "CppName.h"
#include "SrcPos.h"
#include "Auto.h"
#include "Size.h"

class Type;
class Template;
class World;

/**
 * Represents a reference to a type in C++. (ie. whenever we're using a type).
 */
class TypeRef : public Refcount {
public:
	TypeRef(const SrcPos &pos);

	// Position of this type.
	SrcPos pos;

	// Is this type const?
	bool constType;

	// Get the size of this type.
	virtual Size size() const = 0;

	// Is this a gc:d type?
	virtual bool gcType() const = 0;

	// Resolve type info.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const = 0;

	// Print.
	virtual void print(wostream &to) const = 0;

	// Get an ID to sort functions.
	// This does not properly account for all cases to be globally unique, it will, however,
	// give a decent ordering of functions to make cache-locality a bit better when unpacking
	// at least.
	// It is currently constructed as follows:
	// The last two digits indicate modifiers (e.g. templates, references). The upper ones are references.
	virtual size_t sortId() const = 0;
};

inline wostream &operator <<(wostream &to, const TypeRef &c) {
	c.print(to);
	if (c.constType)
		to << L" const";
	return to;
}


/**
 * Generic templated type. Only templates in the last position are supported at the moment.
 */
class TemplateType : public TypeRef {
public:
	TemplateType(const SrcPos &pos, const CppName &name, const vector<Auto<TypeRef>> &params = vector<Auto<TypeRef>>());

	// Name.
	CppName name;

	// Template types.
	vector<Auto<TypeRef>> params;

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};


/**
 * Resolved templated type.
 */
class ResolvedTemplateType : public TypeRef {
public:
	ResolvedTemplateType(const SrcPos &pos, Template *templ, const vector<Auto<TypeRef>> &params);

	// Template type.
	Template *type;

	// Template types.
	vector<Auto<TypeRef>> params;

	// Get size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return true; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};


/**
 * Pointer type.
 */
class PtrType : public TypeRef {
public:
	PtrType(Auto<TypeRef> of);

	// Type.
	Auto<TypeRef> of;

	// Get the size of this type.
	virtual Size size() const { return Size::sPtr; }

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};


/**
 * Ref type.
 */
class RefType : public TypeRef {
public:
	RefType(Auto<TypeRef> of);

	// Type.
	Auto<TypeRef> of;

	// Get the size of this type.
	virtual Size size() const { return Size::sPtr; }

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};


/**
 * Maybe type, for classes (i.e. MAYBE(T *))
 */
class MaybeClassType : public TypeRef {
public:
	MaybeClassType(Auto<TypeRef> of);

	// Type.
	Auto<PtrType> of;

	// Get the size of this type.
	virtual Size size() const { return of->size(); }

	// Is this a gc:d type?
	virtual bool gcType() const { return of->gcType(); }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};

/**
 * Maybe type, for values (i.e. Maybe<T>).
 */
class MaybeValueType : public TypeRef {
public:
	MaybeValueType(Auto<TypeRef> of);

	// Type.
	Auto<TypeRef> of;

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return of->gcType(); }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};


/**
 * Named type.
 */
class NamedType : public TypeRef {
public:
	NamedType(const SrcPos &pos, const CppName &name);
	NamedType(const SrcPos &pos, const String &name);

	// Name.
	CppName name;

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};


/**
 * Resolved type.
 */
class ResolvedType : public TypeRef {
public:
	explicit ResolvedType(Type *type);
	ResolvedType(const TypeRef &templ, Type *type);

	// Type.
	Type *type;

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const;

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};


/**
 * Type built into C++.
 */
class BuiltInType : public TypeRef {
public:
	BuiltInType(const SrcPos &pos, const String &name, Size size);

	// Name.
	String name;

	// Size.
	Size tSize;

	// Get the size of this type.
	virtual Size size() const { return tSize; }

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};


/**
 * Void type.
 */
class VoidType : public TypeRef {
public:
	VoidType(const SrcPos &pos);

	// Get the size of this type.
	virtual Size size() const { return Size(); }

	// Gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};


/**
 * GcArray type.
 */
class GcArrayType : public TypeRef {
public:
	GcArrayType(const SrcPos &pos, Auto<TypeRef> of, bool weak);

	// Type of what?
	Auto<TypeRef> of;

	// Weak?
	bool weak;

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return true; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};


/**
 * GcWatch type.
 */
class GcWatchType : public TypeRef {
public:
	GcWatchType(const SrcPos &pos);

	// Size of this type.
	virtual Size size() const;

	// Gc:d type?
	virtual bool gcType() const { return true; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};


/**
 * EnginePtr type.
 */
class EnginePtrType : public TypeRef {
public:
	EnginePtrType(const SrcPos &pos) : TypeRef(pos) {}

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return false; }

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};

/**
 * GcType type.
 */
class GcTypeType : public TypeRef {
public:
	GcTypeType(const SrcPos &pos) : TypeRef(pos) {}

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const { return true; }


	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Print.
	virtual void print(wostream &to) const;

	// Get sort id.
	virtual size_t sortId() const;
};


class UnknownPrimitive;

/**
 * Unknown type. These are to allow GC:d classes to contain types unknown to the preprocessor
 * without having to allocate them separatly all the time.
 *
 * Supports various kind of external types:
 * PTR_NOGC - pointer to non-gc object.
 * PTR_GC - pointer to gc object.
 * INT - integer sized object.
 *
 * Also: if 'id' is the name of another type, it substitutes that.
 */
class UnknownType : public TypeRef {
public:
	// Create. 'id' is the kind of external type.
	UnknownType(const CppName &id, Auto<TypeRef> of);

	// Type of what?
	Auto<TypeRef> of;

	// Get the size of this type.
	virtual Size size() const;

	// Is this a gc:d type?
	virtual bool gcType() const;

	// Resolve.
	virtual Auto<TypeRef> resolve(World &in, const CppName &context) const;

	// Find an 'unknown' type wrapper for this type.
	Auto<TypeRef> wrapper(World &in) const;

	// Print.
	virtual void print(wostream &to) const;

	// Description of an id.
	struct ID {
		const wchar_t *name;
		const Size &size; // needs to be a reference for some reason.
		bool gc;
	};

	// All known ids.
	static const ID ids[];

	// Get sort id.
	virtual size_t sortId() const;
private:
	// Current id.
	const ID *id;

	// The alternative type we've resolved to.
	Auto<TypeRef> alias;
};


inline bool isGcPtr(Auto<TypeRef> t) {
	if (Auto<PtrType> p = t.as<PtrType>()) {
		return p->of->gcType();
	} else if (Auto<RefType> r = t.as<RefType>()) {
		return r->of->gcType();
	} else if (Auto<MaybeClassType> r = t.as<MaybeClassType>()) {
		return isGcPtr(r->of);
	} else if (Auto<UnknownType> r = t.as<UnknownType>()) {
		return r->gcType();
	} else {
		return false;
	}
}

inline Auto<TypeRef> makeConst(Auto<TypeRef> v) {
	v->constType = true;
	return v;
}
