#include "stdafx.h"
#include "Listing.h"
#include "Exception.h"
#include "UsedRegs.h"
#include "Arena.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Core/CloneEnv.h"

namespace code {

	static const nat invalid = -1;

	wostream &operator <<(wostream &to, FreeOpt o) {
		if ((o & freeOnBoth) == freeOnBoth) {
			to << L"always";
		} else if (o & freeOnBlockExit) {
			to << L"on block exit";
		} else if (o & freeOnException) {
			to << L"on exception";
		} else {
			to << L"never";

			if (o & freeInactive)
				to << L", needs activation";
			return to;
		}

		if (o & freePtr)
			to << L", by ptr";
		else
			to << L", by value";

		if (o & freeInactive)
			to << L", needs activation";

		return to;
	}

	StrBuf &operator <<(StrBuf &to, FreeOpt o) {
		if ((o & freeOnBoth) == freeOnBoth) {
			to << S("always");
		} else if (o & freeOnBlockExit) {
			to << S("on block exit");
		} else if (o & freeOnException) {
			to << S("on exception");
		} else {
			to << S("never");

			if (o & freeInactive)
				to << S(", needs activation");
			return to;
		}

		if (o & freePtr)
			to << S(", by ptr");
		else
			to << S(", by value");

		if (o & freeInactive)
			to << S(", needs activation");

		return to;
	}

	Listing::Entry::Entry() : instr(null), labels(null) {}

	Listing::Entry::Entry(Instr *i) : instr(i), labels(null) {}

	Listing::Entry::Entry(Instr *i, Array<Label> *l) : instr(i), labels(l) {}

	void Listing::Entry::deepCopy(CloneEnv *env) {
		// Instr is immutable. No need to do anything with that!

		if (labels)
			// No need to call 'deepCopy' here, as labels are simple enough.
			labels = new (labels) Array<Label>(*labels);
	}

	void Listing::Entry::add(Engine &e, Label l) {
		if (l == Label())
			return;

		if (!labels)
			labels = new (e) Array<Label>(l);
		else
			labels->push(l);
	}

	StrBuf &operator <<(StrBuf &to, Listing::Entry e) {
		if (e.labels != null && e.labels->any()) {
			StrBuf *tmp = new (&to) StrBuf();
			*tmp << e.labels->at(0);

			for (nat i = 1; i < e.labels->count(); i++)
				*tmp << L", " << width(3) << e.labels->at(i);

			*tmp << L": ";

			to << width(20) << tmp->toS();
		} else {
			to << width(20) << L" ";
		}

		if (e.instr)
			to << e.instr;

		return to;
	}

	/**
	 * Variable information.
	 */

	Listing::VarInfo::VarInfo(Str *name, Type *type, Bool ref, SrcPos pos)
		: name(name), type(type), ref(ref), pos(pos) {}

	void Listing::VarInfo::deepCopy(CloneEnv *env) {
		// No need.
	}


	/**
	 * Internal storage.
	 */

	Listing::IVar::IVar(Nat parent, Size size, TypeDesc *param, Operand freeFn, FreeOpt opt) :
		parent(parent), size(size), param(param), info(null), freeFn(freeFn), freeOpt(opt) {}

	void Listing::IVar::deepCopy(CloneEnv *env) {
		if (info) {
			info = new (env) VarInfo(*info);
			info->deepCopy(env);
		}
	}

	Listing::IBlock::IBlock(Engine &e)
		: parent(Block().id), vars(new (e) Array<Nat>()), catchInfo(null) {}

	Listing::IBlock::IBlock(Engine &e, Nat parent)
		: parent(parent), vars(new (e) Array<Nat>()), catchInfo(null) {}

	void Listing::IBlock::deepCopy(CloneEnv *env) {
		vars = new (vars) Array<Nat>(*vars);
		if (catchInfo) {
			// No need for recursive deep copies.
			catchInfo = new (catchInfo) Array<CatchInfo>(*catchInfo);
		}
	}


	/**
	 * Listing.
	 */

	Listing::Listing() : arena(null) {
		init(false, new (this) PrimitiveDesc(Primitive()));
	}

	Listing::Listing(Bool member, TypeDesc *result) : arena(null) {
		init(member, result);
	}

	Listing::Listing(const Arena *arena) : arena(arena) {
		init(false, new (this) PrimitiveDesc(Primitive()));
	}

	Listing::Listing(const Arena *arena, Bool member, TypeDesc *result) : arena(arena) {
		init(member, result);
	}

	void Listing::init(Bool member, TypeDesc *result) {
		this->code = new (this) Array<Entry>();
		this->nextLabels = null;
		this->nextLabel = 1;
		this->params = new (this) Array<Nat>();
		this->vars = new (this) Array<IVar>();
		this->blocks = new (this) Array<IBlock>();
		this->ehClean = false;
		this->ehCatch = false;
		this->member = member;
		this->result = result;

		// Create the root block.
		blocks->push(IBlock(engine()));
	}

	Listing::Listing(const Listing &o) :
		result(o.result),
		member(o.member),
		arena(o.arena),
		code(o.code),
		nextLabels(o.nextLabels),
		params(o.params),
		vars(o.vars),
		blocks(o.blocks),
		ehClean(o.ehClean),
		ehCatch(o.ehCatch) {

		deepCopy(new (this) CloneEnv());
	}

	void Listing::deepCopy(CloneEnv *env) {
		cloned(result, env);
		code = new (this) Array<Entry>(*code);
		code->deepCopy(env);
		if (nextLabels) {
			nextLabels = new (this) Array<Label>(*nextLabels);
			nextLabels->deepCopy(env);
		}
		params = new (this) Array<Nat>(*params);
		params->deepCopy(env);
		vars = new (this) Array<IVar>(*vars);
		vars->deepCopy(env);
		blocks = new (this) Array<IBlock>(*blocks);
		blocks->deepCopy(env);
	}

	Listing *Listing::createShell() const {
		return createShell(arena);
	}

	Listing *Listing::createShell(const Arena *arena) const {
		Listing *shell = arena ? new (this) Listing(arena) : new (this) Listing();

		shell->result = result;
		shell->member = member;
		shell->nextLabel = nextLabel;
		if (nextLabels)
			shell->nextLabels = new (this) Array<Label>(*nextLabels);
		shell->params = new (this) Array<Nat>(*params);
		shell->vars = new (this) Array<IVar>(*vars);
		shell->blocks = new (this) Array<IBlock>(*blocks);
		shell->ehClean = ehClean;
		shell->ehCatch = ehCatch;

		// Make sure we make recursive copies.
		shell->deepCopy(new (this) CloneEnv());

		return shell;
	}

	Listing &Listing::operator <<(Instr *i) {
		code->push(Entry(i, nextLabels));
		nextLabels = null;
		return *this;
	}

	Listing &Listing::operator <<(Label l) {
		if (!nextLabels)
			nextLabels = new (this) Array<Label>();
		nextLabels->push(l);
		return *this;
	}

	Listing &Listing::operator <<(MAYBE(Array<Label> *) l) {
		if (l)
			for (Nat i = 0; i < l->count(); i++)
				*this << l->at(i);

		return *this;
	}

	MAYBE(Array<Label> *) Listing::labels(Nat id) const {
		if (id == count())
			return nextLabels;
		else
			return code->at(id).labels;
	}

	Label Listing::meta() {
		return Label(0);
	}

	Label Listing::label() {
		return Label(nextLabel++);
	}

	Nat Listing::labelCount() const {
		return nextLabel;
	}

	Block Listing::root() const {
		return Block(0);
	}

	Block Listing::createBlock(Block parent) {
		if (parent.id >= blocks->count())
			return Block();

		Nat blockId = blocks->count();
		blocks->push(IBlock(engine(), parent.id));
		return Block(blockId);
	}

	Nat Listing::findId(Array<Nat> *in, Nat val) {
		for (nat i = 0; i < in->count(); i++)
			if (in->at(i) == val)
				return i;
		return in->count();
	}

	Var Listing::createVar(Nat index) const {
		return Var(index, vars->at(index).size);
	}

	Var Listing::prev(Var v) const {
		if (v.id >= vars->count())
			return Var();

		if (isParam(v)) {
			// Find the previous parameter:
			Nat id = findId(params, v.id);
			if (id > 0 && id < params->count())
				return createVar(params->at(id - 1));
			else
				return Var();
		}

		// Must be a variable somewhere.
		IVar &var = vars->at(v.id);
		IBlock &block = blocks->at(var.parent);

		Nat id = findId(block.vars, v.id);
		if (id > 0) {
			// Previous variable in here!
			assert(id < block.vars->count());
			return createVar(block.vars->at(id - 1));
		}

		// We need to find a block with variables in it!
		for (Block at = Block(block.parent); at != Block(); at = parent(at)) {
			IBlock &now = blocks->at(at.id);
			if (now.vars->any())
				return createVar(now.vars->last());
		}

		// None found. Try parameters:
		if (params->any())
			return createVar(params->last());

		// Nope. No more candidates then!
		return Var();
	}

	void Listing::moveParam(Var v, Nat to) {
		if (!isParam(v))
			return;

		Nat pos = 0;
		for (nat i = 0; i < params->count(); i++) {
			if (params->at(i) == v.id) {
				pos = i;
				break;
			}
		}

		params->remove(pos);
		params->insert(to, v.id);
	}

	void Listing::moveFirst(Var v) {
		if (isParam(v))
			return;

		if (v.id >= vars->count())
			return;

		Nat blockId = vars->at(v.id).parent;
		IBlock &block = blocks->at(blockId);
		for (Nat i = block.vars->count() - 1; i > 0; i--) {
			if (block.vars->at(i) == v.id) {
				// Move it ahead one step. We'll find it again in the next iteration!
				block.vars->at(i) = block.vars->at(i - 1);
				block.vars->at(i - 1) = v.id;
			}
		}
	}

	Block Listing::parent(Block b) const {
		if (b.id >= blocks->count())
			return Block();

		IBlock &block = blocks->at(b.id);
		return Block(block.parent);
	}

	Block Listing::parent(Var v) const {
		if (v.id >= vars->count())
			return Block();

		return Block(vars->at(v.id).parent);
	}

	Bool Listing::accessible(Var v, Block b) const {
		if (v.id >= vars->count())
			return false;
		if (b.id >= blocks->count())
			return false;

		// Parameters are always visible.
		if (isParam(v))
			return true;

		IVar &var = vars->at(v.id);
		return isParent(Block(var.parent), b);
	}

	Bool Listing::isParent(Block parent, Block q) const {
		if (parent.id >= blocks->count())
			return false;
		if (q.id >= blocks->count())
			return false;

		for (Block at = q; at != Block(); at = this->parent(at)) {
			if (at == parent)
				return true;
		}

		return false;
	}

	Bool Listing::isParam(Var v) const {
		if (v.id >= vars->count())
			return false;

		return vars->at(v.id).param != null;
	}

	MAYBE(TypeDesc *) Listing::paramDesc(Var v) const {
		if (v.id >= vars->count())
			return null;

		return vars->at(v.id).param;
	}

	MAYBE(Listing::VarInfo *) Listing::varInfo(Var v) const {
		if (v.id >= vars->count())
			return null;

		return vars->at(v.id).info;
	}

	void Listing::varInfo(Var v, MAYBE(VarInfo *) info) {
		if (v.id < vars->count())
			vars->at(v.id).info = info;
	}

	Operand Listing::freeFn(Var v) const {
		if (v.id >= vars->count())
			return Operand();

		return vars->at(v.id).freeFn;
	}

	FreeOpt Listing::freeOpt(Var v) const {
		if (v.id >= vars->count())
			return freeOnNone;

		return vars->at(v.id).freeOpt;
	}

	void Listing::freeOpt(Var v, FreeOpt opt) {
		if (v.id >= vars->count())
			return;

		vars->at(v.id).freeOpt = opt;
	}

	static bool checkFree(Engine &e, const Operand &free, FreeOpt &when) {
		if (when & freePtr)
			if (max(free.size(), Size::sLong) != Size::sLong)
				throw new (e) InvalidValue(S("Can not destroy values larger than 8 bytes by value."));

		if (!free.empty())
			if (when & freeOnException)
				return true;

		if (when & freeInactive)
			when = freeInactive;
		else
			when = freeOnNone;
		return false;
	}

	Var Listing::createVar(Block in, Size size, Operand free, FreeOpt when) {
		if (in.id >= blocks->count())
			return Var();

		if (checkFree(engine(), free, when))
			ehClean = true;

		Nat id = vars->count();
		vars->push(IVar(in.id, size, null, free, when));

		IBlock &block = blocks->at(in.id);
		block.vars->push(id);

		return Var(id, size);
	}

	Var Listing::createVar(Block in, TypeDesc *type) {
		return createVar(in, type, freeDef);
	}

	Var Listing::createVar(Block in, TypeDesc *type, FreeOpt when) {
		if (ComplexDesc *c = as<ComplexDesc>(type)) {
			return createVar(in, type->size(), c->dtor, when | freePtr);
		} else {
			return createVar(in, type->size(), Operand(), when);
		}
	}

	Var Listing::createParam(TypeDesc *type) {
		return createParam(type, freeDef);
	}

	Var Listing::createParam(TypeDesc *type, FreeOpt when) {
		if (ComplexDesc *c = as<ComplexDesc>(type)) {
			return createParam(type, c->dtor, when | freePtr);
		} else {
			return createParam(type, Operand(), freeDef);
		}
	}

	Var Listing::createParam(TypeDesc *type, Operand free) {
		return createParam(type, free, freeDef);
	}

	Var Listing::createParam(TypeDesc *type, Operand free, FreeOpt when) {
		if (checkFree(engine(), free, when))
			ehClean = true;

		Nat id = vars->count();
		vars->push(IVar(invalid, type->size(), type, free, when));

		params->push(id);
		return Var(id, type->size());
	}

	Array<Block> *Listing::allBlocks() const {
		Array<Block> *r = new (this) Array<Block>();
		r->reserve(blocks->count());
		for (Nat i = 0; i < blocks->count(); i++) {
			r->push(Block(i));
		}
		return r;
	}

	Array<Var> *Listing::allVars() const {
		Array<Var> *r = new (this) Array<Var>();
		r->reserve(vars->count());
		for (nat i = 0; i < vars->count(); i++)
			r->push(createVar(i));
		return r;
	}

	Array<Var> *Listing::allVars(Block b) const {
		Array<Var> *r = new (this) Array<Var>();
		if (b.id >= blocks->count())
			return r;

		IBlock &block = blocks->at(b.id);

		// The parameters are technically in the root block. Add them first, it makes more sense.
		if (b == root()) {
			r->reserve(block.vars->count() + params->count());
			for (Nat i = 0; i < params->count(); i++)
				r->push(createVar(params->at(i)));
		}

		// Regular variables.
		r->reserve(block.vars->count());
		for (Nat i = 0; i < block.vars->count(); i++)
			r->push(createVar(block.vars->at(i)));

		return r;
	}

	Array<Var> *Listing::allParams() const {
		Array<Var> *r = new (this) Array<Var>();
		for (nat i = 0; i < params->count(); i++)
			r->push(createVar(params->at(i)));
		return r;
	}

	Listing::CatchInfo::CatchInfo(Type *type, Label resume) : type(type), resume(resume) {}

	void Listing::addCatch(Block block, CatchInfo add) {
		if (block.id >= blocks->count())
			return;

		Array<CatchInfo> *&info = blocks->at(block.id).catchInfo;
		if (!info)
			info = new (this) Array<CatchInfo>();
		info->push(add);

		ehCatch = true;
	}

	void Listing::addCatch(Block block, Type *type, Label resume) {
		addCatch(block, CatchInfo(type, resume));
	}

	MAYBE(Array<Listing::CatchInfo> *) Listing::catchInfo(Block block) const {
		if (block.id >= blocks->count())
			return null;

		return blocks->at(block.id).catchInfo;
	}

	static Str *toS(Engine &e, Array<Label> *l) {
		if (!l)
			return new (e) Str(L"");

		StrBuf *to = new (e) StrBuf();
		for (nat i = 0; i < l->count(); i++) {
			if (i > 0)
				*to << L", ";
			*to << l->at(i);
		}
		*to << L": ";
		return to->toS();
	}

	void Listing::toS(StrBuf *to) const {
		if (params->any())
			*to << L"Parameters:\n";

		for (nat i = 0; i < params->count(); i++) {
			Nat id = params->at(i);
			putVar(*to, id);
		}

		putBlock(*to, 0);

		UsedRegs r = usedRegs(arena, this);
		*to << L"Dirty registers: " << r.all;

		for (nat i = 0; i < code->count(); i++) {
			Entry &e = code->at(i);

			*to << L"\n";
			if (e.labels != null && e.labels->any()) {
				*to << code::toS(engine(), e.labels) << L"\n";
			}
			*to << width(20) << r.used->at(i)->toS();
			*to << L" | " << e.instr;
		}
	}

	void Listing::putBlock(StrBuf &to, Nat block) const {
		to << L"Block " << block << L" {\n";
		{
			storm::Indent z(&to);

			IBlock &b = blocks->at(block);

			if (b.catchInfo) {
				for (nat i = 0; i < b.catchInfo->count(); i++) {
					CatchInfo info = b.catchInfo->at(i);
					to << L"Goto " << info.resume << L" on " << runtime::typeName(info.type) << L"\n";
				}
			}

			for (Nat i = 0; i < b.vars->count(); i++) {
				putVar(to, b.vars->at(i));
			}

			// Find and print all children.
			for (Nat i = 0; i < blocks->count(); i++) {
				if (blocks->at(i).parent == block)
					putBlock(to, i);
			}

		}
		to << L"}\n";
	}

	void Listing::putVar(StrBuf &to, Nat var) const {
		IVar &v = vars->at(var);
		to << width(3) << var
		   << S(": size ") << v.size
		   << S(" free ") << v.freeOpt
		   << S(" using ") << v.freeFn;
		if (v.info)
			to << S(" (name: ") << v.info->name << S(")");
		to << S("\n");
	}

}
