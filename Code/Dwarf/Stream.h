#pragma once
#include "Utils/Bitwise.h"
#include "Registers.h"

namespace code {
	namespace dwarf {

		/**
		 * Definitions of some useful DWARF constants.
		 */
#define DW_CFA_advance_loc        0x40 // + delta
#define DW_CFA_offset             0x80 // + register
#define DW_CFA_restore            0xC0 // + register
#define DW_CFA_nop                0x00
#define DW_CFA_set_loc            0x01
#define DW_CFA_advance_loc1       0x02
#define DW_CFA_advance_loc2       0x03
#define DW_CFA_advance_loc4       0x04
#define DW_CFA_offset_extended    0x05
#define DW_CFA_restore_extended   0x06
#define DW_CFA_undefined          0x07
#define DW_CFA_same_value         0x08
#define DW_CFA_register           0x09
#define DW_CFA_remember_state     0x0a
#define DW_CFA_restore_state      0x0b
#define DW_CFA_def_cfa            0x0c
#define DW_CFA_def_cfa_register   0x0d
#define DW_CFA_def_cfa_offset     0x0e
#define DW_CFA_def_cfa_expression 0x0f
#define DW_CFA_expression         0x10
#define DW_CFA_offset_extended_sf 0x11
#define DW_CFA_def_cfa_sf         0x12
#define DW_CFA_def_cfa_offset_sf  0x13
#define DW_CFA_val_offset         0x14
#define DW_CFA_val_offset_sf      0x15
#define DW_CFA_val_expression     0x16

#define DW_EH_PE_absptr 0x00

		/**
		 * Output to a buffer in DWARF compatible format.
		 */
		struct DStream {
			byte *to;
			nat pos;
			nat len;

			// Initialize.
			DStream(byte *to, nat len) : to(to), pos(0), len(len) {}
			DStream(byte *to, nat len, nat pos) : to(to), pos(pos), len(len) {}

			// Did we run out of space?
			bool overflow() const {
				return pos > len;
			}

			// Write a byte.
			void putByte(byte b) {
				if (pos < len)
					to[pos] = b;
				pos++;
			}

			// Write a pointer.
			void putPtr(const void *value) {
				if (pos + sizeof(value) <= len) {
					const void **d = (const void **)&to[pos];
					*d = value;
				}
				pos += sizeof(value);
			}

			// Write an unsigned number (encoded as LEB128).
			void putUNum(nat value) {
				while (value >= 0x80) {
					putByte((value & 0x7F) | 0x80);
					value >>= 7;
				}
				putByte(value & 0x7F);
			}

			// Write a signed number (encoded as LEB128).
			void putSNum(int value) {
				nat src = value;
				nat bits = value;
				if (value < 0) {
					bits = ~bits; // make 'positive'
					bits <<= 1; // make sure to get at least one sign bit in the output.
				}

				while (bits >= 0x80) {
					putByte((src & 0x7F) | 0x80);
					src >>= 7;
					bits >>= 7;
				}
				putByte(src & 0x7F);
			}

			// Write OP-codes.
			void putOp(byte op) {
				putByte(op);
			}
			void putSOp(byte op, int p1) {
				putByte(op);
				putSNum(p1);
			}
			void putUOp(byte op, nat p1) {
				putByte(op);
				putUNum(p1);
			}
			void putSOp(byte op, int p1, int p2) {
				putByte(op);
				putSNum(p1);
				putSNum(p2);
			}
			void putUOp(byte op, nat p1, nat p2) {
				putByte(op);
				putUNum(p1);
				putUNum(p2);
			}

			// Encode the 'advance_loc' op-code.
			void putAdvance(nat bytes) {
				if (bytes <= 0x3F) {
					putByte(DW_CFA_advance_loc + bytes);
				} else if (bytes <= 0xFF) {
					putByte(DW_CFA_advance_loc1);
					putByte(bytes);
				} else if (bytes <= 0xFFFF) {
					putByte(DW_CFA_advance_loc2);
					putByte(bytes & 0xFF);
					putByte(bytes >> 8);
				} else {
					putByte(DW_CFA_advance_loc4);
					putByte(bytes & 0xFF);
					putByte((bytes & 0xFF00) >> 8);
					putByte((bytes & 0xFF0000) >> 16);
					putByte(bytes >> 24);
				}
			}
		};

		class FDEStream : public DStream {
		public:
			FDEStream(FDE *to, Nat &pos) : DStream(to->data, FDE_DATA), dest(pos) {
				this->pos = dest;
			}

			~FDEStream() {
				dest = pos;
				dbg_assert(!overflow(), L"Increase FDE_DATA to at least " + ::toS(roundUp(pos, nat(sizeof(void *)))) + L"!");
			}

			Nat &dest;
		};

	}
}
