#include "stdafx.h"
#include "Compiler/Package.h"

void compileNoRes(Package *in) {
	in->forceLoad();
	for (NameSet::Iter i = in->begin(); i != in->end(); i++) {
		if (Package *child = as<Package>(i.v())) {
			if (child->name->endsWith(S("_res")) || *child->name == S("res")) {
			} else {
				compileNoRes(child);
			}
		} else {
			i.v()->compile();
		}
	}
}

/**
 * Compile all code and see what happens.
 */

BEGIN_TEST(Core, Compile) {
	Engine &e = gEngine();
	CHECK_RUNS(e.package(S("core"))->compile());
} END_TEST

BEGIN_TEST(Lang, Compile) {
	Engine &e = gEngine();
	CHECK_RUNS(e.package(S("lang"))->compile());
} END_TEST

BEGIN_TEST(Util, Compile) {
	Engine &e = gEngine();
	CHECK_RUNS(e.package(S("util"))->compile());
} END_TEST

BEGIN_TEST(Demo, Compile) {
	Engine &e = gEngine();
	CHECK_RUNS(e.package(S("demo"))->compile());
} END_TEST

BEGIN_TEST(LayoutLib, Compile) {
	Engine &e = gEngine();
	CHECK_RUNS(e.package(S("layout"))->compile());
} END_TEST

BEGIN_TEST(Presentation, Compile) {
	Engine &e = gEngine();
	CHECK_RUNS(e.package(S("presentation"))->compile());
} END_TEST

BEGIN_TEST(Ui, Compile) {
	Engine &e = gEngine();
	CHECK_RUNS(e.package(S("ui"))->compile());
} END_TEST

BEGIN_TEST(Progvis, Compile) {
	Engine &e = gEngine();
	CHECK_RUNS(e.package(S("progvis"))->compile());
} END_TEST

BEGIN_TEST(SQL, Compile) {
	Engine &e = gEngine();
	CHECK_RUNS(e.package(S("sql"))->compile());
} END_TEST

BEGIN_TEST(Parser, Compile) {
	Engine &e = gEngine();
	CHECK_RUNS(e.package(S("parser"))->compile());
} END_TEST

BEGIN_TEST(Markdown, Compile) {
	Engine &e = gEngine();
	CHECK_RUNS(compileNoRes(e.package(S("markdown"))));
} END_TEST

BEGIN_TEST(Tutorials, Compile) {
	Engine &e = gEngine();
	CHECK_RUNS(compileNoRes(e.package(S("tutorials"))));
} END_TEST
