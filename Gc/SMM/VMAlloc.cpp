#include "stdafx.h"
#include "VMAlloc.h"

#if STORM_GC == STORM_GC_SMM

#include "Gc/Gc.h"
#include "Util.h"

namespace storm {
	namespace smm {

		struct ChunkCompare {
			inline bool operator() (const Chunk &a, const Chunk &b) const {
				return size_t(a.at) < size_t(b.at);
			}
		};

		VMAlloc::VMAlloc(size_t initSize) :
			vm(VM::create(this)), pageSize(vm->pageSize),
			minAddr(0), maxAddr(1),
			info(null), lastAlloc(0) {

			dbg_assert((1 << identifierBits) ==  (1 + infoData(0xFF)), L"Invalid value of 'identifierBits' found in Config.h!");

			size_t granularity = max(vmAllocMinSize, vm->allocGranularity);

			// Try to allocate the initial block!
			initSize = roundUp(initSize, granularity);
			void *mem = vm->reserve(null, initSize);
			while (!mem) {
				initSize /= 2;
				if (initSize < granularity)
					throw BasicGcError(L"Unable to allocate memory for the initial arena!");
				mem = vm->reserve(null, initSize);
			}
			addReserved(Chunk(mem, initSize));
		}

		VMAlloc::~VMAlloc() {
			for (size_t i = 0; i < reserved.size(); i++)
				vm->free(reserved[i].at, reserved[i].size);

			delete vm;
		}

		void VMAlloc::updateMinMax(size_t &minAddr, size_t &maxAddr) {
			if (reserved.empty()) {
				minAddr = 0;
				maxAddr = 1;
			}

			minAddr = size_t(reserved[0].at);
			maxAddr = minAddr + reserved[0].size;

			for (size_t i = 1; i < reserved.size(); i++) {
				size_t base = size_t(reserved[i].at);
				minAddr = min(minAddr, base);
				maxAddr = max(maxAddr, base + reserved[i].size);
			}
		}

		void VMAlloc::addReserved(Chunk chunk) {
			insertSorted(reserved, chunk, ChunkCompare());

			if (info) {
				resizeInfo(chunk);
			} else {
				createInfo(chunk);
			}
		}

		void VMAlloc::createInfo(Chunk chunk) {
			// New allocation, just put it in the beginning of the first block. This will always
			// work since the info table will always be smaller than the one single allocation.

			updateMinMax(minAddr, maxAddr);
			size_t count = infoCount(minAddr, maxAddr);

			vm->commit(chunk.at, roundUp(count, vmAllocMinSize));
			info = (byte *)chunk.at;
			memset(info, INFO_FREE, count);

			// Mark the new information struct as 'allocated' so that we don't overwrite it later!
			for (size_t at = infoOffset(info), to = infoOffset(info + count - 1); at <= to; at++) {
				info[at] = INFO_USED_INTERNAL;
			}
		}

		void VMAlloc::markInaccessible(void *from, void *to) {
			if (size_t(from) >= size_t(to))
				return;

			for (size_t at = infoOffset(from), end = infoOffset((byte *)to - 1); at <= end; at++) {
				info[at] = INFO_USED_INTERNAL;
			}
		}

		void VMAlloc::resizeInfo(Chunk chunk) {
			// Note: This is a bit tricky. Since allocations may not be aligned to our
			// granularity by the underlying system's memory manager, we might have to adjust
			// 'min' and 'max' a bit so that the alignment of the old data structure remains
			// correct. (Example: old allocation was at 0x20000, new allocation is at 0x11000
			// with an alignment of 0x10000 will require poking 'min' to 0x10000, otherwise
			// blocks in the old allocation are shifted by 0x1000). This will sadly waste some
			// of the virtual address space, but that is fine as we don't expect to have many
			// disjoint blocks of memory to manage.

			// We have two options here:
			// 1: If the old info allocation contains enough space to accommodate the new
			//    allocation (or if we can trivially expand it), we re-use it. This might
			//    require moving the data already there if the new block was 'before'
			//    the old limits.
			// 2: If that is not possible, we allocate space for a new info block in the
			//    new allocation and copy the data there.

			// Compute the new bounds.
			size_t newMin, newMax;
			updateMinMax(newMin, newMax);

			// Modify 'min' to preserve block alignment. We don't need to do anything with 'max',
			// rounding will handle that.
			{
				size_t currentAlignment = minAddr % vmAllocMinSize;
				size_t alignedMin = newMin - (newMin % vmAllocMinSize) + currentAlignment;
				if (alignedMin < newMin)
					alignedMin += vmAllocMinSize;

				newMin = alignedMin;
			}

			size_t oldCount = infoCount(minAddr, maxAddr);
			size_t newCount = infoCount(newMin, newMax);

			// Mark the old info table as 'free', so that we can try to re-allocate it.
			for (size_t at = infoOffset(info), to = infoOffset(info + oldCount - 1); at <= to; at++) {
				info[at] = INFO_FREE;
			}

			// Examine if the new size fits inside the old allocation.
			bool reuseOld = true;
			for (size_t at = infoOffset(info), to = infoOffset(info + newCount - 1); at <= to; at++) {
				if (info[at] != INFO_FREE)
					reuseOld = false;
			}

			if (reuseOld) {
				// We have room to expand the current allocation. Shuffle the data around!
				// Note: If 'min' expands, it always shrinks. Therefore, we can always copy
				// data from high indexes to low indexes to avoid destroying the old data.
				dbg_assert(newMin <= minAddr, L"Min address should always shrink!");

				size_t delta = (minAddr - newMin) / vmAllocMinSize;
				for (size_t i = oldCount; i > 0; i--) {
					info[i + delta - 1] = info[i - 1];
				}

				// Initialize the remainder of the table.
				for (size_t i = 0; i < delta; i++) {
					info[i] = INFO_FREE;
				}
				for (size_t i = delta + oldCount; i < delta + newCount; i++) {
					info[i] = INFO_FREE;
				}

			} else {
				// Insert the table at the start of the new allocation. That is empty anyway. We
				// assume it is large enough. This is likely always the case, as the table is
				// compact. For 32-bit systems, we only ever need 64k (i.e. this case currently
				// never happens as blocks are 64k minimum). For 64-bit systems with 48 bit virtual
				// addresses, we need 2^32 bytes at a maximum, which could be a problem. However, we
				// have other problems if our allocations are that spread out...
				dbg_assert(chunk.size >= newCount, L"Not enough space for bookkeeping data!");

				byte *newInfo = (byte *)chunk.at;
				memset(newInfo, INFO_FREE, newCount);

				// Copy the old data over to its new location.
				if (minAddr > newMin) {
					size_t delta = (minAddr - newMin) / vmAllocMinSize;
					memcpy(newInfo + delta, info, min(oldCount, newCount - delta));
				} else {
					size_t delta = (newMin - minAddr) / vmAllocMinSize;
					memcpy(newInfo, info + delta, min(oldCount - delta, newCount));
				}
			}

			// Start using the new info table.
			minAddr = newMin;
			maxAddr = newMax;

			// Mark the newly allocated chunk as 'free'. It might previously have been a part of
			// memory that was a 'hole' between two other allocations. We might mark a bit too much
			// in this loop, but that is fine since we will explicitly mark any 'holes' as
			// unavailable later on.
			for (size_t at = infoOffset(chunk.at); at < infoOffset(chunk.end()); at++) {
				info[at] = INFO_FREE;
			}

			// Mark the new allocation as 'used' (might be partially done already).
			for (size_t at = infoOffset(info), to = infoOffset(info + newCount - 1); at <= to; at++) {
				info[at] = INFO_USED_INTERNAL;
			}

			// Mark all holes as inaccessible. We don't need to care about the ranges, since that
			// will be covered by 'addrMin' and 'addrMax'.
			for (size_t i = 1; i < reserved.size(); i++) {
				markInaccessible(reserved[i - 1].end(), reserved[i].at);
			}
		}

		size_t VMAlloc::totalPieces() const {
			size_t pieces = 0;
			for (size_t i = 0; i < reserved.size(); i++) {
				pieces += reserved[i].size >> vmAllocBits;
			}
			return pieces;
		}

		bool VMAlloc::expandAlloc(size_t minPieces) {
			// Attempt to double our reserved range if we are able to.
			size_t maxPieces = max(minPieces * 10, totalPieces());

			for (size_t attempt = maxPieces; attempt >= minPieces; attempt /= 2) {
				Chunk alloc = attemptAlloc(attempt);
				if (!alloc.empty()) {
					// Success!
					addReserved(alloc);
					return true;
				}
			}

			return false;
		}

		Chunk VMAlloc::attemptAlloc(size_t pieces) {
			size_t bytes = roundUp(pieces*vmAllocMinSize, vm->allocGranularity);

			// Try between blocks.
			for (size_t i = 1; i < reserved.size(); i++) {
				Chunk start = reserved[i - 1];
				Chunk end = reserved[i];

				size_t hole = (byte *)end.at - (byte *)start.end();
				if (hole >= bytes) {
					// Small enough to attempt to fill it entirely?
					if (hole / 2 < bytes) {
						if (void *mem = vm->reserve(start.end(), hole))
							return Chunk(mem, hole);
					}

					// Start of the hole.
					if (void *mem = vm->reserve(start.end(), bytes))
						return Chunk(mem, bytes);

					// End of the hole.
					if (void *mem = vm->reserve((byte *)end.at - bytes, bytes))
						return Chunk(mem, bytes);
				}
			}

			// After the last block.
			if (void *mem = vm->reserve(reserved.back().end(), bytes))
				return Chunk(mem, bytes);

			// Before the first block.
			if (void *mem = vm->reserve((byte *)reserved.front().at - bytes, bytes))
				return Chunk(mem, bytes);

			// Anywhere.
			if (void *mem = vm->reserve(null, bytes))
				return Chunk(mem, bytes);

			// Failure.
			return Chunk();
		}

		struct PieceRange {
			size_t start;
			size_t count;
		};

		static inline PieceRange findRange(byte *in, size_t from, size_t to, size_t min, size_t preferred) {
			PieceRange candidate = { from, 0 };
			size_t start = from;

			for (size_t i = from; i < to; i++) {
				if (in[i] & 0x03) {
					// Not free...
					start = i + 1;
				} else {
					size_t count = i - start + 1;
					if (count >= candidate.count) {
						candidate.start = start;
						candidate.count = count;
					}

					if (candidate.count >= preferred) {
						return candidate;
					}
				}
			}

			// Largest chunk was too small. Indicate failure.
			if (candidate.count < min)
				candidate.count = 0;

			return candidate;
		}

		Chunk VMAlloc::alloc(size_t size, byte identifier) {
			return alloc(size, size, identifier);
		}

		Chunk VMAlloc::alloc(size_t minSize, size_t preferredSize, byte identifier) {
			byte packedIdentifier = (identifier & 0x3F) << 2;
			dbg_assert(infoData(packedIdentifier) == identifier, L"Identifier too large!");

			size_t minPieces = (minSize + vmAllocMinSize - 1) / vmAllocMinSize;
			size_t preferredPieces = (preferredSize + vmAllocMinSize - 1) / vmAllocMinSize;

			size_t wrap = infoCount();
			PieceRange range = findRange(info, lastAlloc, wrap, minPieces, preferredPieces);
			if (range.count != preferredPieces) {
				size_t upperBound = min(wrap, lastAlloc + minPieces);
				PieceRange alternative = findRange(info, 0, upperBound, minPieces, preferredPieces);

				if (alternative.count > range.count)
					range = alternative;

				if (alternative.count == 0) {
					// Out of memory! Try to expand our allocated range!
					if (!expandAlloc(preferredPieces))
						return Chunk();

					wrap = infoCount();
					range = findRange(info, 0, wrap, minPieces, preferredPieces);

					// Still not enough? If so, we give up.
					if (range.count == 0)
						return Chunk();
				}
			}

			lastAlloc = range.start + range.count;

			// Mark the memory as in use, and potentially changed.
			memset(info + range.start, INFO_USED_WRITTEN | packedIdentifier, range.count);

			Chunk mem(infoPtr(range.start), range.count*vmAllocMinSize);
			vm->commit(mem.at, mem.size);

			return mem;
		}

		void VMAlloc::free(Chunk chunk) {
			// Mark as free.
			memset(info + infoOffset(chunk.at), INFO_FREE, chunk.size / vmAllocMinSize);

			// Decommit from OS.
			vm->decommit(chunk.at, chunk.size);
		}

		void VMAlloc::watchWrites(Chunk chunk) {
			size_t first = infoOffset(chunk.at);
			size_t pieces = (size_t(chunk.end()) - size_t(infoPtr(first)) + vmAllocMinSize - 1) / vmAllocMinSize;

			bool any = false;
			for (size_t i = first; i < first + pieces; i++) {
				any |= infoWritten(info[i]);

				// Remove the 'modified' flag.
				info[i] &= ~0x02;
			}

			// And set memory protection if needed.
			if (any)
				vm->watchWrites(infoPtr(first), pieces*vmAllocMinSize);
		}

		void VMAlloc::stopWatchWrites(Chunk chunk) {
			size_t first = infoOffset(chunk.at);
			size_t pieces = (size_t(chunk.end()) - size_t(infoPtr(first)) + vmAllocMinSize - 1) / vmAllocMinSize;

			bool any = false;
			for (size_t i = first; i < first + pieces; i++) {
				any |= infoProtected(info[i]);
				info[i] |= 0x02;
			}

			// Remove memory protection if needed.
			if (any)
				vm->stopWatchWrites(infoPtr(first), pieces*vmAllocMinSize);
		}

		bool VMAlloc::anyWrites(Chunk chunk) {
			size_t first = infoOffset(chunk.at);
			size_t pieces = (size_t(chunk.end()) - size_t(infoPtr(first)) + vmAllocMinSize - 1) / vmAllocMinSize;

			// TODO: A fenwick tree would speed up this operation quite a bit if the chunk is large.
			for (size_t i = first; i < first + pieces; i++)
				if (infoWritten(info[i]))
					return true;

			return false;
		}

		bool VMAlloc::onProtectedWrite(void *addr) {
			// Outside of memory managed by us?
			if (size_t(addr) < minAddr || size_t(addr) >= maxAddr)
				return false;

			size_t offset = infoOffset(addr);

			// Check so that it is allocated and protected.
			if (infoProtected(info[offset])) {
				// Then, unprotect, and mark as changed.
				vm->stopWatchWrites(infoPtr(offset), vmAllocMinSize);
				info[offset] |= INFO_USED_WRITTEN;
				return true;
			}

			return false;
		}

		void VMAlloc::fillSummary(MemorySummary &summary) const {
			size_t infoSz = infoCount() * sizeof(*info);
			summary.bookkeeping += infoSz;
			summary.fragmented += roundUp(infoSz, vmAllocMinSize) - infoSz;

			for (size_t i = 0; i < reserved.size(); i++)
				summary.reserved += reserved[i].size;
		}

		void VMAlloc::dbg_dump() {
			const char *mode[4];
			mode[INFO_FREE] = "free";
			mode[INFO_USED_CLIENT] = "client";
			mode[INFO_USED_WRITTEN] = "client, written";
			mode[INFO_USED_INTERNAL] = "internal";

			for (size_t i = 0; i < infoCount(); i++) {
				PLN(infoPtr(i) << L" - " << infoPtr(i + 1) << L": " << infoData(info[i]) << L" " << mode[info[i] & 0x03]);
			}
		}

	}
}

#endif
