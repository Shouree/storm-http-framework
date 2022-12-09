#include "stdafx.h"
#include "StackInfo.h"
#include <typeinfo>
#include <iomanip>

void StackInfo::alloc(StackFrame *frames, nat count) const {}

void StackInfo::free(StackFrame *frames, nat count) const {}

void StdOutput::nextFrame() {
	if (frameNumber > 0)
		to << std::endl;
	to << std::setw(3) << (frameNumber++) << L": ";
}
