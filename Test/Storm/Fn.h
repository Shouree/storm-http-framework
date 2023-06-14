#pragma once
#include "Compiler/Engine.h"
#include "OS/FnCall.h"

template <class Res>
inline Res runFn(const wchar *name) {
	Engine &e = gEngine();
	SimpleName *sName = parseSimpleName(e, name);
	Function *f = as<Function>(e.scope().find(sName));
	assert(f, L"Function " + ::toS(sName) + L" not found!");
	storm::Value r(StormInfo<Res>::type(e));
	assert(r.mayReferTo(f->result), L"Invalid return type of " + ::toS(sName) + L"!");
	os::FnCall<Res> c = os::fnCall();
	return c.call(f->ref().address(), false);
}

template <>
inline void runFn(const wchar *name) {
	Engine &e = gEngine();
	SimpleName *sName = parseSimpleName(e, name);
	Function *f = as<Function>(e.scope().find(sName));
	assert(f, L"Function " + ::toS(sName) + L" not found!");
	assert(f->result == storm::Value(), L"Invalid return type of " + ::toS(sName) + L"!");
	os::FnCall<void> c = os::fnCall();
	return c.call(f->ref().address(), false);
}

template <class Res, class T>
inline Res runFn(const wchar *name, T t) {
	Engine &e = gEngine();
	SimpleName *sName = parseSimpleName(e, name);
	sName->last()->params->push(storm::Value(StormInfo<T>::type(e)));
	Function *f = as<Function>(e.scope().find(sName));
	assert(f, L"Function " + ::toS(sName) + L" not found!");
	storm::Value r(StormInfo<Res>::type(e));
	assert(r.mayReferTo(f->result), L"Invalid return type of " + ::toS(sName) + L"!");
	os::FnCall<Res> c = os::fnCall().add(t);
	return c.call(f->ref().address(), false);
}

template <class Res, class T, class U>
inline Res runFn(const wchar *name, T t, U u) {
	Engine &e = gEngine();
	SimpleName *sName = parseSimpleName(e, name);
	sName->last()->params->push(storm::Value(StormInfo<T>::type(e)));
	sName->last()->params->push(storm::Value(StormInfo<U>::type(e)));
	Function *f = as<Function>(e.scope().find(sName));
	assert(f, L"Function " + ::toS(sName) + L" not found!");
	storm::Value r(StormInfo<Res>::type(e));
	assert(r.mayReferTo(f->result), L"Invalid return type of " + ::toS(sName) + L"!");
	os::FnCall<Res> c = os::fnCall().add(t).add(u);
	return c.call(f->ref().address(), false);
}


template <class Res>
inline Res runFnUnsafe(const wchar *name) {
	Engine &e = gEngine();
	SimpleName *sName = parseSimpleName(e, name);
	Function *f = as<Function>(e.scope().find(sName));
	assert(f, L"Function " + ::toS(sName) + L" not found!");
	os::FnCall<Res> c = os::fnCall();
	return c.call(f->ref().address(), false);
}
