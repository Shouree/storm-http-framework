#include "stdafx.h"
#include "Code/CallCommon.h"
#include "Code/TypeDesc.h"

using namespace code;

SimpleDesc *tinyIntDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sInt * 2, 2);
	desc->at(0) = Primitive(primitive::integer, Size::sInt, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sInt, Offset::sInt);
	return desc;
}

SimpleDesc *smallIntDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sPtr * 2, 2);
	desc->at(0) = Primitive(primitive::integer, Size::sPtr, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sPtr, Offset::sPtr);
	return desc;
}

SimpleDesc *largeIntDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sPtr * 3, 3);
	desc->at(0) = Primitive(primitive::integer, Size::sPtr, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sPtr, Offset::sPtr);
	desc->at(2) = Primitive(primitive::integer, Size::sPtr, Offset::sPtr * 2);
	return desc;
}

SimpleDesc *mixedDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sPtr + Size::sFloat * 2, 3);
	desc->at(0) = Primitive(primitive::integer, Size::sPtr, Offset());
	desc->at(1) = Primitive(primitive::real, Size::sFloat, Offset::sPtr);
	desc->at(2) = Primitive(primitive::real, Size::sFloat, Offset::sPtr + Offset::sFloat);
	return desc;
}

SimpleDesc *bytesDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sByte * 2, 2);
	desc->at(0) = Primitive(primitive::integer, Size::sByte, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sByte, Offset::sByte);
	return desc;
}

SimpleDesc *pointDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sFloat * 2, 2);
	desc->at(0) = Primitive(primitive::real, Size::sFloat, Offset());
	desc->at(1) = Primitive(primitive::real, Size::sFloat, Offset::sFloat);
	return desc;
}

SimpleDesc *rectDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sFloat * 4, 4);
	desc->at(0) = Primitive(primitive::real, Size::sFloat, Offset());
	desc->at(1) = Primitive(primitive::real, Size::sFloat, Offset::sFloat);
	desc->at(2) = Primitive(primitive::real, Size::sFloat, Offset::sFloat * 2);
	desc->at(3) = Primitive(primitive::real, Size::sFloat, Offset::sFloat * 3);
	return desc;
}
