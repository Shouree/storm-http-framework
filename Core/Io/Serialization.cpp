#include "stdafx.h"
#include "Serialization.h"
#include "Str.h"
#include "Exception.h"

namespace storm {

	/**
	 * Object descriptions.
	 */

	static void checkCtor(Type *t, FnBase *ctor) {
		// TODO: Check!
	}

	SerializedType::SerializedType(Type *t, FnBase *ctor)
		: type(t), readCtor(ctor), mySuper(null) {

		init();
	}

	SerializedType::SerializedType(Type *t, FnBase *ctor, Type *super)
		: type(t), readCtor(ctor), mySuper(super) {

		init();
	}

	void SerializedType::init() {
		checkCtor(type, readCtor);
		types = new (this) Array<TObject *>();
	}

	typeInfo::TypeInfo SerializedType::info() const {
		typeInfo::TypeInfo r = baseInfo();
		if (!runtime::isValue(type))
			r |= typeInfo::classType;
		return r;
	}

	typeInfo::TypeInfo SerializedType::baseInfo() const {
		return typeInfo::custom;
	}

	void SerializedType::toS(StrBuf *to) const {
		*to << S("Serialization info for ") << runtime::typeName(type) << S(":");
		if (mySuper) {
			*to << S("\n  super: ") << runtime::typeName(mySuper);
		}
		*to << S("\n  constructor: ") << readCtor;
	}

	SerializedType::Cursor::Cursor() : type(null), pos(1) {}

	SerializedType::Cursor::Cursor(SerializedType *type) : type(type), pos(0) {
		if (!type->super())
			pos++;
	}

	void SerializedType::Cursor::next() {
		if (!any())
			return;
		if (++pos == type->types->count() + 1)
			pos = type->typesRepeat + 1;
	}


	SerializedMember::SerializedMember(Str *name, Type *type) : name(name), type(type) {}

	SerializedStdType::SerializedStdType(Type *t, FnBase *ctor)
		: SerializedType(t, ctor), names(new (engine()) Array<Str *>()) {}

	SerializedStdType::SerializedStdType(Type *t, FnBase *ctor, Type *parent)
		: SerializedType(t, ctor, parent), names(new (engine()) Array<Str *>()) {}

	void SerializedStdType::add(Str *name, Type *type) {
		typeAdd(type);
		names->push(name);
		typeRepeat(typeCount());
	}

	void SerializedStdType::add(const wchar *name, Type *type) {
		add(new (this) Str(name), type);
	}

	SerializedMember SerializedStdType::at(Nat i) const {
		return SerializedMember(names->at(i), typeAt(i));
	}

	typeInfo::TypeInfo SerializedStdType::baseInfo() const {
		return typeInfo::none;
	}

	void SerializedStdType::toS(StrBuf *to) const {
		SerializedType::toS(to);
		for (Nat i = 0; i < names->count(); i++)
			*to << S("\n  ") << names->at(i) << S(": ") << runtime::typeName(typeAt(i));
	}


	SerializedTuples::SerializedTuples(Type *t, FnBase *ctor) : SerializedType(t, ctor) {
		typeAdd(StormInfo<Nat>::type(engine()));
		typeRepeat(typeCount());
	}

	SerializedTuples::SerializedTuples(Type *t, FnBase *ctor, Type *super) : SerializedType(t, ctor) {
		typeAdd(StormInfo<Nat>::type(engine()));
		typeRepeat(typeCount());
	}

	typeInfo::TypeInfo SerializedTuples::baseInfo() const {
		return typeInfo::tuple;
	}

	void SerializedTuples::toS(StrBuf *to) const {
		SerializedType::toS(to);
		for (Nat i = 0; i < count(); i++)
			*to << S("\n  tuple ") << i << S(": ") << runtime::typeName(at(i));
	}


	SerializedMaybe::SerializedMaybe(Type *t, FnBase *ctor, Type *contained) : SerializedType(t, ctor) {
		typeAdd(StormInfo<Bool>::type(engine()));
		typeAdd(contained);
		typeRepeat(1);
	}

	void SerializedMaybe::toS(StrBuf *to) const {
		SerializedType::toS(to);
		*to << S("\n  maybe: ") << runtime::typeName(contained());
	}

	typeInfo::TypeInfo SerializedMaybe::baseInfo() const {
		return typeInfo::maybe;
	}


	/**
	 * ObjIStream
	 */

	ObjIStream::Member::Member(Str *name, Nat type) : name(name), type(type), read(0) {}

	ObjIStream::Member::Member(const Member &o, Int read) : name(o.name), type(o.type), read(read) {}


	ObjIStream::Desc::Desc(Byte flags, Nat parent, Str *name) : data(Nat(flags) << 24), parent(parent) {
		members = new (this) Array<Member>();

		Type *t = runtime::fromIdentifier(name);
		if (!t)
			throw new (this) SerializationFormatError(TO_S(this, S("Unknown type: ") << demangleName(name)));
		const Handle &h = runtime::typeHandle(t);
		if (!h.serializedTypeFn)
			throw new (this) SerializationFormatError(TO_S(this, S("The type ") << demangleName(name) << S(" is not serializable.")));

		info = (*h.serializedTypeFn)();
	}

	ObjIStream::Desc::Desc(Byte flags, Type *type, FnBase *ctor) : data(Nat(flags) << 24), parent(0) {
		members = null;
		info = new (this) SerializedType(type, ctor);
	}

	Nat ObjIStream::Desc::findMember(Str *name) const {
		for (Nat i = 0; i < members->count(); i++)
			if (*members->at(i).name == *name)
				return i;
		return members->count();
	}


	ObjIStream::Cursor::Cursor() : desc(null), tmp(null), pos(0) {}

	ObjIStream::Cursor::Cursor(Desc *desc) : desc(desc), tmp(null), pos(0) {
		Nat entries = desc->storage();
		if (entries > 0) {
			// TODO: Maybe cache the array type somewhere?
			Engine &e = desc->engine();
			tmp = runtime::allocArray<Variant>(e, StormInfo<Variant>::handle(e).gcArrayType, entries);
		}
	}

	const ObjIStream::Member &ObjIStream::Cursor::current() const {
		return desc->members->at(pos);
	}

	void ObjIStream::Cursor::next() {
		if (!any())
			return;

		pos++;

		// Repeat if we're a tuple!
		if (desc->isTuple() && pos == desc->members->count())
			pos = 1;
	}

	void ObjIStream::Cursor::pushTemporary(const Variant &v) {
		tmp->v[tmp->filled++] = v;
	}


	/**
	 * Size limits.
	 */

	// Check if 'base + add' would be larger than 'limit'. Updates 'base' to reflect the addition,
	// but caps it at 'limit' to avoid overflows.
	static bool addSize(Nat &base, Nat add, Nat limit) {
		// Note: It is safe to rely on overflow here since unsigned types are defined to behave as
		// modular arithmetics.
		Nat original = base;
		base += add;
		if (base > limit || base < original) {
			base = limit;
			return true;
		}
		return false;
	}

	static void throwLimitError(Engine &e, Nat limit) {
		throw new (e) SizeLimitReached(S("type metadata"), 0, limit);
	}

	void ObjIStream::maxTypeDescSize(Nat limit) {
		typeDescLimit = limit;
		if (typeDescCurrent >= limit)
			throwLimitError(engine(), limit);
	}

	void ObjIStream::checkTypeDescSize(Nat add) {
		if (addSize(typeDescCurrent, add, typeDescLimit))
			throwLimitError(engine(), typeDescLimit);
	}

	static void throwSizeError(Engine &e, Nat alloc, Nat limit) {
		throw new (e) SizeLimitReached(S("an object"), alloc, limit);
	}

	void ObjIStream::checkAllocSize(Nat add) {
		if (add > readSizeBudget)
			throwSizeError(engine(), add, readSizeBudget);
		readSizeBudget -= add;
	}

	static void throwArraySizeError(Engine &e, Nat alloc, Nat limit) {
		throw new (e) SizeLimitReached(S("an array"), alloc, limit);
	}

	void ObjIStream::checkArrayAlloc(Nat elemSize, Nat count) {
		Word totalSize = Word(elemSize) * Word(count);
		if (totalSize > maxArraySize)
			throwArraySizeError(engine(), Nat(min(Word(0xFFFFFFFF), totalSize)), maxArraySize);

		if (totalSize > readSizeBudget)
			throwArraySizeError(engine(), Nat(totalSize), readSizeBudget);

		readSizeBudget -= totalSize;
	}

	/**
	 * IStream.
	 */

	template <class T, T (IStream::* readFn)()>
	static void CODECALL read(T *out, ObjIStream *from) {
		new (out) T((from->from->*readFn)());
		from->end();
	}

	static void readStr(Str **out, ObjIStream *from) {
		new (Place(out)) Str(from);
	}

	ObjIStream::ObjIStream(IStream *src)
		: from(src),
		  maxReadSize(0xFFFFFFFF),
		  maxArraySize(0xFFFFFFFF),
		  typeDescLimit(0xFFFFFFFF),
		  typeDescCurrent(0),
		  readSizeBudget(0) {

		clearObjects();

		depth = new (this) Array<Cursor>();
		typeIds = new (this) Map<Nat, Desc *>();

#define ADD_BUILTIN(id, t)												\
		typeIds->put(id, new (this) Desc(								\
						typeInfo::none,									\
						StormInfo<t>::type(e),							\
						new (e) FnBase(address(&read<t, &IStream::read ## t>), null, false, null)))

		// Add built-in types here to avoid special cases later on.
		Engine &e = engine();
		ADD_BUILTIN(boolId, Bool);
		ADD_BUILTIN(byteId, Byte);
		ADD_BUILTIN(intId, Int);
		ADD_BUILTIN(natId, Nat);
		ADD_BUILTIN(longId, Long);
		ADD_BUILTIN(wordId, Word);
		ADD_BUILTIN(floatId, Float);
		ADD_BUILTIN(doubleId, Double);

#undef ADD_BUILTIN

		// String is a special case.
		typeIds->put(strId, new (this) Desc(typeInfo::classType, StormInfo<Str>::type(e),
														new (e) FnBase(address(&readStr), null, false, null)));
	}

	void ObjIStream::readValue(Type *type, PTR_GC out) {
		Info info = start();
		if (info.result.any()) {
			// TODO: Check type!
			info.result.moveValue(out);
			return;
		}

		Desc *expected = findInfo(info.expectedType);
		if (!expected->isValue())
			throw new (this) SerializationFormatError(S("Expected a value type, but got a class type."));
		if (expected->info->type != type) {
			Str *msg = TO_S(this, S("Type mismatch. Expected ") << runtime::typeName(expected->info->type)
							<< S(" but got ") << runtime::typeName(type) << S("."));
			throw new (this) SerializationFormatError(msg);
		}

		readValueI(expected, out);
	}

	Object *ObjIStream::readClass(Type *type) {
		// Note: the type represented by 'expectedId' may not always exactly match that of 'type'
		// since the types used at the root of deserialization do not need to be equal. However,
		// 'type' needs to be a parent of whatever we find as 'actual' later on, which is a subclass
		// of 'expected'.
		Info info = start();
		if (info.result.any()) {
			RootObject *result = info.result.getObject();
			if (!runtime::isA(result, type)) {
				StrBuf *msg = new (this) StrBuf();
				*msg << S("Attempted to read an object of type: ")
					 << runtime::typeName(type)
					 << S(" but received an object of type ")
					 << runtime::typeName(runtime::typeOf(result))
					 << S(".");
				throw new (this) SerializationFormatError(msg->toS());
			}
			return (Object *)result;
		}

		Desc *expected = findInfo(info.expectedType);

		if (expected->isValue()) {
			// If this type has turned into a class type, just go ahead. There is no problem with
			// this as long as the members match.
			Object *created = (Object *)runtime::allocObject(0, expected->info->type);
			readValueI(expected, created);
			return created;
		}

		return readClassI(expected, type);
	}

	void ObjIStream::readPrimitiveValue(StoredId id, PTR_GC out) {
		Info info = start();
		Desc *expected = findInfo(info.expectedType);
		if (!expected->isValue())
			throw new (this) SerializationFormatError(S("Expected a value type, but got a class type."));
		if (id != info.expectedType)
			throw new (this) SerializationFormatError(S("Mismatch of built-in types!"));

		if (info.result.any()) {
			// TODO: Check type!
			info.result.moveValue(out);
			return;
		}

		readValueI(expected, out);
	}

	Object *ObjIStream::readPrimitiveObject(StoredId id) {
		Info info = start();
		Desc *expected = findInfo(info.expectedType);
		if (expected->isValue())
			throw new (this) SerializationFormatError(S("Expected a class type, but got a value type."));
		if (id != info.expectedType)
			throw new (this) SerializationFormatError(S("Mismatch of built-in types!"));

		if (info.result.any()) {
			// TODO: Check type?
			return (Object *)info.result.getObject();
		}

		Object *created = (Object *)runtime::allocObject(0, expected->info->type);
		readValueI(expected, created);
		return created;
	}

	Variant ObjIStream::readClass(Nat typeId) {
		Desc *type = findInfo(typeId);
		if (type->isValue()) {
			Type *t = type->info->type;
			if (runtime::isValue(t)) {
				Variant v = Variant::uninitializedValue(t);
				readValueI(type, v.getValue());
				v.valueInitialized();
				return v;
			} else {
				// We support turning values into classes.
				Object *created = (Object *)runtime::allocObject(0, t);
				readValueI(type, created);
				return Variant(created);
			}
		} else {
			return Variant(readClassI(type, type->info->type));
		}
	}

	void ObjIStream::readValueI(Desc *type, void *out) {
		// Find the parent classes and push them on the stack to keep track of what we're doing.
		Desc *d = type;
		while (d != null) {
			depth->push(Cursor(d));

			if (d->parent)
				d = findInfo(d->parent);
			else
				d = null;
		}

		// Call the constructor!
		ObjIStream *me = this;
		os::FnCall<void, 2> call = os::fnCall().add(out).add(me);
		type->info->readCtor->callRaw(call, null, null);
	}

	Object *ObjIStream::readClassI(Desc *expected, Type *t) {
		// Did we encounter this instance before?
		Nat objId = from->readNat();
		if (Object *old = objIds->get(objId, null)) {
			if (!runtime::isA(old, t))
				throw new (this) SerializationFormatError(S("Wrong type found during deserialization."));
			return old;
		}

		// Read the actual type.
		Nat actualId = from->readNat();
		Desc *actual = findInfo(actualId);

		// Make sure the type we're about to create is appropriate.
		if (!runtime::isA(actual->info->type, t))
			throw new (this) SerializationFormatError(S("Wrong type found during deserialization."));

		// Check so that we are not over capacity.
		Type *type = actual->info->type;
		checkAllocSize(runtime::typeGc(type)->stride);

		// Allocate the object and deserialize by calling the constructor, just like we do with value types.
		Object *created = (Object *)runtime::allocObject(0, type);
		objIds->put(objId, created);

		readValueI(actual, created);

		return created;
	}

	ObjIStream::Info ObjIStream::start() {
		Info r = { 0, Variant() };

		if (depth->empty()) {
			// First type.

			// Reset current size limit.
			readSizeBudget = maxReadSize;

			// Read its id from the stream.
			if (!from->more())
				throw new (this) EndOfStream();

			// Check so that we have some data. "more" might turn "false" first when we try to read stuff.
			GcPreArray<Byte, 1> d;
			Buffer b = from->peek(emptyBuffer(d));
			if (b.empty())
				throw new (this) EndOfStream();

			r.expectedType = from->readNat();
			return r;
		}

		// Some other type. Examine what we expect to read.
		Cursor &at = depth->last();
		if (at.customDesc())
			throw new (this) SerializationFormatError(S("Can not use 'start' when serializing custom types."));

		// Process objects until we find something we can return!
		while (true) {
			if (!at.any())
				throw new (this) SerializationFormatError(S("Trying to deserialize too many members."));

			const Member &expected = at.current();
			at.next();

			if (expected.read < 0) {
				// Read one object into temporary storage and continue.
				at.pushTemporary(readClass(expected.type));
			} else if (expected.read > 0) {
				r.expectedType = expected.type;
				r.result = at.temporary(Nat(expected.read - 1));
				return r;
			} else {
				// Read it now!
				r.expectedType = expected.type;
				return r;
			}
		}
	}

	void ObjIStream::end() {
		if (depth->empty())
			throw new (this) SerializationFormatError(S("Mismatched calls to startX during dedeserialization!"));

		Cursor end = depth->last();
		if (!end.atEnd())
			throw new (this) SerializationFormatError(S("Missing fields during deserialization!"));

		depth->pop();

		if (depth->empty()) {
			// Last one, clear the object cache.
			clearObjects();
		}
	}

	ObjIStream::Desc *ObjIStream::findInfo(Nat id) {
		Desc *result = typeIds->get(id, null);
		if (result)
			return result;

		Byte flags = from->readByte();
		Str *name = Str::read(from, typeDescLimit - typeDescCurrent);
		Nat parent = from->readNat();

		checkTypeDescSize(sizeof(Desc) + name->peekLength()*sizeof(wchar));
		result = new (this) Desc(flags, parent, name);

		if (flags & typeInfo::tuple) {
			// Tuple.
			checkTypeDescSize(sizeof(Member));
			result->members->push(Member(null, natId));

			for (Nat type = from->readNat(); type != endId; type = from->readNat()) {
				checkTypeDescSize(sizeof(Member));
				result->members->push(Member(null, type));
			}

			validateTuple(result);
		} else if (flags & typeInfo::maybe) {
			// Maybe-type.
			checkTypeDescSize(sizeof(Member)*2);
			result->members->push(Member(null, boolId));
			result->members->push(Member(null, from->readNat()));

			validateMaybe(result);
		} else if (flags & typeInfo::custom) {
			// Nothing to read for custom types. Indicate the absence of known serialization by
			// setting 'members' to 'null'.
			result->members = null;
		} else {
			// Members.
			for (Nat type = from->readNat(); type != endId; type = from->readNat()) {
				Str *name = Str::read(from, typeDescLimit - typeDescCurrent);
				checkTypeDescSize(sizeof(Member) + name->peekLength()*sizeof(wchar));
				result->members->push(Member(name, type));
			}

			validateMembers(result);
		}

		typeIds->put(id, result);
		return result;
	}

	void ObjIStream::validateMembers(Desc *stream) {
		SerializedStdType *our = as<SerializedStdType>(stream->info);
		if (!our)
			throw new (this) SerializationFormatError(S("Trying to deserialize a standard type into a non-compatible type!"));

		// Note: We check the parent type when reading objects. Otherwise, we need quite a bit of
		// bookkeeping to know when to validate parents of all types unless we want to check all
		// types every time a new type is found in the stream.

		// Note: We can not check the types of members here, as all member types are not necessarily
		// known at this point. This is instead done when 'readXxx' is called.

		// Check the members we need to find, and match them to the members in the stream. During
		// the process, figure out how to use the temporary storage to store some members there if
		// necessary.
		Nat streamPos = 0;
		Map<Str *, Member> *tempPos = new (this) Map<Str *, Member>();
		// Note: Original count, not updated when we grow the array. This is intentional in order to
		// not catch the duplicate entries referring to temporary storage!
		Nat memberCount = stream->members->count();
		for (Nat i = 0; i < our->count(); i++) {
			SerializedMember ourMember = our->at(i);

			// Look until we find it in the stream, saving intermediate members to temporary storage.
			while (streamPos < memberCount) {
				Member &m = stream->members->at(streamPos);
				if (*m.name == *ourMember.name)
					break;

				// Store it in temporary storage and remember its location (indexed from 1).
				m.read = -1;
				tempPos->put(m.name, Member(m, tempPos->count() + 1));
				streamPos++;
			}

			if (streamPos < memberCount) {
				// If we do, we can read it without temporary storage.
				stream->members->at(streamPos).read = 0;
				streamPos++;
			} else if (tempPos->has(ourMember.name)) {
				// If we skipped it earlier, read it from temporary storage.
				stream->members->push(tempPos->get(ourMember.name));
			} else {
				// Otherwise, an error.

				// TODO: When we have support for default initialization, take that into consideration here!
				Str *msg = TO_S(this, S("The member ") << ourMember.name << S(", required for type ")
								<< runtime::typeName(our->type) << S(", is not present in the stream."));
				throw new (this) SerializationFormatError(msg);
			}
		}

		// Remember the number of bytes required in temporary storage.
		stream->storage(tempPos->count());

		// PLN(L"Members of " << runtime::typeName(our->type));
		// for (Nat i = 0; i < stream->members->count(); i++) {
		// 	Member t = stream->members->at(i);

		// 	PLN(L"  " << t.name << L", " << t.type << L", " << t.read);
		// }
		// PLN(L"  (" << stream->storage() << L" temporary entries required)");
	}

	void ObjIStream::validateTuple(Desc *stream) {
		SerializedTuples *our = as<SerializedTuples>(stream->info);
		if (!our)
			throw new (this) SerializationFormatError(S("Trying to deserialize a type type into a non-compatible type!"));

		// We don't try to do anything intelligent here. We just check so that the number of
		// elements in each tuple match.
		Nat tupleCount = stream->members->count() - 1;
		if (our->count() != tupleCount) {
			Str *msg = TO_S(this, S("Tuple size mismatch. Stream: ") << tupleCount
							<< S(", here: ") << our->count() << S("."));
			throw new (msg) SerializationFormatError(msg);
		}
	}

	void ObjIStream::validateMaybe(Desc *stream) {
		SerializedMaybe *our = as<SerializedMaybe>(stream->info);
		if (!our)
			throw new (this) SerializationFormatError(S("Trying to deserialize a type into a non-compatible type!"));

		// Nothing more to verify.
	}

	void ObjIStream::clearObjects() {
		objIds = new (this) Map<Nat, Object *>();
	}


	/**
	 * ObjOStream
	 */

	static const Nat typeMask = 0x80000000;

	ObjOStream::ObjOStream(OStream *to) : to(to) {
		clearObjects();

		depth = new (this) Array<SerializedType::Cursor>();
		typeIds = new (this) Map<TObject *, Nat>();
		nextId = firstCustomId;
		serializedTypes = new (this) Map<TObject *, SerializedType *>();

		// Insert the standard types inside 'ids', so we don't have to bother with them later.
		Engine &e = engine();
		typeIds->put((TObject *)StormInfo<Bool>::type(e), boolId);
		typeIds->put((TObject *)StormInfo<Byte>::type(e), byteId);
		typeIds->put((TObject *)StormInfo<Int>::type(e), intId);
		typeIds->put((TObject *)StormInfo<Nat>::type(e), natId);
		typeIds->put((TObject *)StormInfo<Long>::type(e), longId);
		typeIds->put((TObject *)StormInfo<Word>::type(e), wordId);
		typeIds->put((TObject *)StormInfo<Float>::type(e), floatId);
		typeIds->put((TObject *)StormInfo<Double>::type(e), doubleId);
		typeIds->put((TObject *)StormInfo<Str>::type(e), strId);
	}

	void ObjOStream::clearObjects() {
		MapBase *t = new (this) MapBase(StormInfo<TObject>::handle(engine()), StormInfo<Nat>::handle(engine()));
		objIds = (Map<Object *, Nat> *)t;
	}

	Nat ObjOStream::typeId(Type *type) {
		// TODO: We need to deal with containers somehow.
		TObject *t = (TObject *)type;
		Nat id = typeIds->get(t, nextId);
		if (id == nextId) {
			nextId++;
			id |= typeMask;
			typeIds->put(t, id);
		}
		return id;
	}

	Bool ObjOStream::startValue(Type *type) {
		return startValue(findSerialized(type));
	}

	Bool ObjOStream::startClass(Type *type, const Object *obj) {
		return startClass(findSerialized(type), obj);
	}

	SerializedType *ObjOStream::findSerialized(Type *type) {
		// For convenience in other parts of the implementation.
		if (!type)
			return null;

		Map<TObject *, SerializedType *>::Iter found = serializedTypes->find((TObject *)type);
		if (found != serializedTypes->end())
			return found.v();

		const Handle &h = runtime::typeHandle(type);
		if (!h.serializedTypeFn)
			throw new (this) SerializationFormatError(TO_S(this, S("The type ") << runtime::typeName(type) << S(" is not serializable.")));

		SerializedType *info = (*h.serializedTypeFn)();
		serializedTypes->put((TObject *)type, info);
		return info;
	}

	Bool ObjOStream::startValue(SerializedType *type) {
		Type *expected = start(type);
		if (expected && expected != type->type) {
			// We can assume that the actual type is exactly what we're expecting since value types
			// are sliced. Therefore, we don't need to search for the proper type as we need to do
			// for classes.
			// PVAR(runtime::typeName(expected));
			Str *msg = TO_S(this, S("Unexpected value type during serialization. Expected: ") << runtime::typeName(expected));
			throw new (this) SerializationFormatError(msg);
		}

		if (type->info() & typeInfo::classType)
			throw new (this) SerializationFormatError(S("Expected a class type, but a value type was provided!"));

		writeInfo(type);
		return true;
	}

	Bool ObjOStream::startClass(SerializedType *type, const Object *v) {
		Type *expected = start(type);
		if (expected) {
			// This is the start of a new type.

			// Find the expected type from the description. It should be a direct or indirect parent!
			SerializedType *expectedDesc = type;
			while (expectedDesc && expectedDesc->type != expected)
				expectedDesc = findSerialized(expectedDesc->super());
			if (!expectedDesc)
				throw new (this) SerializationFormatError(S("The provided type description does not match the serialized object."));

			writeInfo(expectedDesc);

			// Now, the reader knows what we're talking about. Now we can bother with references...
			Nat objId = objIds->get((Object *)v, objIds->count());
			to->writeNat(objId);

			if (objId != objIds->count()) {
				// Already existing object?

				// Discard the serialization step for that. If we call 'end', an exception will be thrown.
				depth->pop();
				return false;
			}

			// New object. Write its actual type.
			to->writeNat(typeId(type->type) & ~typeMask);
			objIds->put((Object *)v, objId);
		} else {
			// This is the parent of an object we're already serializing. We don't need any
			// additional headers for that, just possibly the class description.
		}

		if ((type->info() & typeInfo::classType) != typeInfo::classType)
			throw new (this) SerializationFormatError(S("Expected a value type, but a class type was provided."));

		writeInfo(type);
		return true;
	}

	void ObjOStream::startPrimitive(StoredId id) {
		if (depth->empty()) {
			to->writeNat(id & ~typeMask);
		} else {
			depth->last().next();
		}

		depth->push(SerializedType::Cursor());
	}

	Type *ObjOStream::start(SerializedType *type) {
		Type *r = type->type;
		if (depth->empty()) {
			// We're the root object, write a header.
			to->writeNat(typeId(type->type) & ~typeMask);
		} else {
			SerializedType::Cursor &at = depth->last();
			if (!at.any())
				throw new (this) SerializationFormatError(S("Trying to serialize too many fields."));

			if (at.isParent())
				r = null;
			else
				r = at.current();
			at.next();
		}

		// Add a cursor to 'depth' to keep track of what we're doing!
		depth->push(SerializedType::Cursor(type));
		return r;
	}

	void ObjOStream::end() {
		if (depth->empty())
			throw new (this) SerializationFormatError(S("Mismatched calls to startX during serialization!"));

		SerializedType::Cursor end = depth->last();
		if (!end.atEnd())
			throw new (this) SerializationFormatError(S("Missing fields during serialization!"));

		depth->pop();

		if (depth->empty()) {
			// Last one, clear the object cache.
			clearObjects();
		}
	}

	void ObjOStream::writeInfo(SerializedType *t) {
		// Already written?
		Nat id = typeId(t->type);
		if ((id & typeMask) == 0)
			return;

		typeIds->put((TObject *)t->type, id & ~typeMask);

		to->writeByte(Byte(t->info()));
		runtime::typeIdentifier(t->type)->write(to);

		if (t->super()) {
			to->writeNat(typeId(t->super()) & ~typeMask);
		} else {
			to->writeNat(endId);
		}

		if (SerializedStdType *s = as<SerializedStdType>(t)) {
			// Members.
			for (Nat i = 0; i < s->count(); i++) {
				const SerializedMember &member = s->at(i);

				Nat id = typeId(member.type);
				to->writeNat(id & ~typeMask);
				member.name->write(to);
			}

			// End of members.
			to->writeNat(endId);
		} else if (SerializedTuples *tuples = as<SerializedTuples>(t)) {
			// Types.
			for (Nat i = 0; i < tuples->count(); i++) {
				Nat id = typeId(tuples->at(i));
				to->writeNat(id & ~typeMask);
			}

			// End.
			to->writeNat(endId);
		} else if (SerializedMaybe *maybe = as<SerializedMaybe>(t)) {
			to->writeNat(typeId(maybe->contained()) & ~typeMask);
		} else {
			// This is an unsupported type. We don't need to write that description.
		}
	}

	Str *demangleName(Str *name) {
		StrBuf *to = new (name) StrBuf();
		Bool addComma = false;
		Bool addDot = false;
		for (Str::Iter i = name->begin(); i != name->end(); ++i) {
			Char ch = i.v();
			Bool comma = false;
			Bool dot = false;

			if (ch == Char(1u)) {
				dot = true;
			} else if (ch == Char(2u)) {
				*to << S("(");
			} else if (ch == Char(3u)) {
				*to << S(")");
			} else if (ch == Char(4u)) {
				comma = true;
			} else if (ch == Char(5u)) {
				*to << S(" &");
				comma = true;
			} else {
				if (addComma)
					*to << S(", ");
				if (addDot)
					*to << S(".");
				*to << ch;
			}

			addComma = comma;
			addDot = dot;
		}
		return to->toS();
	}
}

