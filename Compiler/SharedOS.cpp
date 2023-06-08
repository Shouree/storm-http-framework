#include "stdafx.h"
#include "SharedOS.h"
#include "Core/Str.h"
#include "Core/Io/Stream.h"
#include "Core/PODArray.h"

#if defined(POSIX)
#include <elf.h>
#endif

namespace storm {

	// Fill a structure with data from a stream.
	template <class T>
	static bool fill(IStream *src, T &out) {
		GcPreArray<byte, sizeof(T)> data;
		Buffer r = src->fill(emptyBuffer(data));
		if (r.filled() != sizeof(T))
			return false;
		memcpy(&out, r.dataPtr(), sizeof(T));
		return true;
	}

	// Read data to an array.
	template <class T>
	T *read(IStream *src, Nat count) {
		Nat size = count*sizeof(T);
		Buffer r = src->fill(buffer(src->engine(), size));
		if (r.filled() != size)
			return null;
		return (T *)r.dataPtr();
	}

#if defined(WINDOWS)

	LoadedLib invalidLib = NULL;

	LoadedLib loadLibrary(Url *path) {
		return LoadLibrary(path->format()->c_str());
	}

	void unloadLibrary(LoadedLib lib) {
		FreeLibrary(lib);
	}

	const void *findLibraryFn(LoadedLib lib, const char *name) {
		return GetProcAddress(lib, name);
	}

	// Find the file offset of a specific section.
	bool seekSection(RIStream *stream, size_t address, IMAGE_SECTION_HEADER *sections, Nat numSections) {
		Nat use = numSections;
		for (Nat i = 0; i < numSections; i++) {
			if (sections[i].VirtualAddress > address)
				break;
			use = i;
		}

		if (use >= numSections)
			return false;

		address -= sections[use].VirtualAddress;
		address += sections[use].PointerToRawData;
		stream->seek(address);

		return true;
	}

	static bool hasExport(RIStream *src, const char *findName) {
		IMAGE_DOS_HEADER dosHeader;
		if (!fill(src, dosHeader))
			return false;

		if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE || dosHeader.e_lfanew == 0)
			return false;

		src->seek(dosHeader.e_lfanew);
		IMAGE_NT_HEADERS ntHeader;
		if (!fill(src, ntHeader))
			return false;

		if (ntHeader.Signature != IMAGE_NT_SIGNATURE || ntHeader.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
			return false;

		// Find and load the section table.
		Nat sectionCount = ntHeader.FileHeader.NumberOfSections;
		if (sectionCount > 1000)
			return false;
		IMAGE_SECTION_HEADER *sections = read<IMAGE_SECTION_HEADER>(src, sectionCount);
		if (!sections)
			return false;

		IMAGE_DATA_DIRECTORY &exports = ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
		if (exports.VirtualAddress == 0)
			return false;

		if (!seekSection(src, exports.VirtualAddress, sections, sectionCount))
			return false;
		IMAGE_EXPORT_DIRECTORY exportDir;
		if (!fill(src, exportDir))
			return false;

		// Search for names... TODO: Check the pointer size on 64-bit systems. Could be either 32-bit or 64-bit!
		Nat nameCount = exportDir.NumberOfNames;
		if (!seekSection(src, exportDir.AddressOfNames, sections, sectionCount))
			return false;
		DWORD *names = read<DWORD>(src, nameCount);
		if (!names)
			return false;

		// Iterate through the names and see what we find!
		size_t findSize = strlen(findName);
		for (Nat i = 0; i < nameCount; i++) {
			if (!seekSection(src, names[i], sections, sectionCount))
				return false;
			char *data = read<char>(src, findSize + 1);
			if (!data)
				return false;

			if (strncmp(findName, data, findSize + 1) == 0)
				return true;
		}

		return true;
	}

	bool hasExport(Url *file, const char *name) {
		RIStream *src = file->read()->randomAccess();
		bool r = hasExport(src, name);
		src->close();
		return r;
	}

#elif defined(POSIX)

#if defined(X86)
#define EM_CURRENT EM_386
#define EI_CLASS_CURRENT ELFCLASS32
#define EI_DATA_CURRENT ELFDATA2LSB
#define EI_OSABI_CURRENT ELFOSABI_SYSV
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Sym Elf32_Sym
#elif defined(X64)
#define EM_CURRENT EM_X86_64
#define EI_CLASS_CURRENT ELFCLASS64
#define EI_DATA_CURRENT ELFDATA2LSB
#define EI_OSABI_CURRENT ELFOSABI_SYSV
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Sym Elf64_Sym
#elif defined(ARM64)
#define EM_CURRENT EM_AARCH64
#define EI_CLASS_CURRENT ELFCLASS64
#define EI_DATA_CURRENT ELFDATA2LSB
#define EI_OSABI_CURRENT ELFOSABI_SYSV
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Sym Elf64_Sym
#else
#error "I do not know your current machine type."
#endif

	LoadedLib invalidLib = null;

	LoadedLib loadLibrary(Url *path) {
		LoadedLib r = dlopen(path->format()->utf8_str(), RTLD_NOW | RTLD_LOCAL);
		if (!r) {
			WARNING(L"Failed to load " << path << L": " << String(dlerror()));
		}
		return r;
	}

	void unloadLibrary(LoadedLib lib) {
		dlclose(lib);
	}

	const void *findLibraryFn(LoadedLib lib, const char *name) {
		return dlsym(lib, name);
	}

	static bool findSections(RIStream *file, Elf_Ehdr &header, Elf_Shdr &dynsym, Elf_Shdr &strtab) {
		bool hasDynsym = false;
		bool hasStrtab = false;

		for (nat i = 0; i < header.e_shnum; i++) {
			Elf_Shdr current;
			file->seek(header.e_shoff + i*header.e_shentsize);
			if (!fill(file, current))
				return false;

			if (current.sh_type == SHT_DYNSYM && !hasDynsym) {
				dynsym = current;
				hasDynsym = true;
			}

			if (current.sh_type == SHT_STRTAB && !hasStrtab) {
				strtab = current;
				hasStrtab = true;
			}

			if (hasDynsym && hasStrtab)
				return true;
		}
		return false;
	}

	static Nat strlen(const Buffer &buf, Nat start) {
		for (Nat i = start; i < buf.count(); i++)
			if (buf[i] == 0)
				return i - start;
		return buf.count() - start;
	}

	// Look for the string table offsets of a specified name.
	static void findNames(RIStream *file, Elf_Shdr &strtab, const char *name, PODArray<Word, 10> &result) {
		Nat nameLen = ::strlen(name);

		// We're assuming that 'name', as well as any entries in the ELF file are way less than 1024 bytes long.
		const Nat chunkSize = 1024;
		Buffer buf = buffer(file->engine(), chunkSize * 2);

		// Fill up the buffer.
		file->seek(strtab.sh_offset);
		file->read(buf);

		// Start looking for strings!
		Word pos = 0;
		Nat bufpos = 0;
		while (pos < strtab.sh_size) {
			Nat len = strlen(buf, bufpos);

			if (nameLen == len) {
				// Could be the one!
				if (strcmp(name, (const char *)buf.dataPtr() + bufpos) == 0)
					result.push(pos);
			}

			pos += len + 1;
			bufpos += len + 1;

			if (bufpos >= chunkSize) {
				// Move data.
				for (Nat i = 0; i < chunkSize; i++)
					buf[i] = buf[i + chunkSize];
				bufpos -= chunkSize;
				// Read another chunk from disk.
				buf.filled(chunkSize);
				file->read(buf);
			}
		}
	}

	struct Verify {
		nat key, value;
	};

	static bool hasExport(RIStream *file, const char *name) {
		// NOTE: We're assuming 64-bit ELF files.
		Elf_Ehdr header;
		if (!fill(file, header))
			return false;

		Verify verify[] = {
			{ EI_MAG0, ELFMAG0 },
			{ EI_MAG1, ELFMAG1 },
			{ EI_MAG2, ELFMAG2 },
			{ EI_MAG3, ELFMAG3 },
			{ EI_CLASS, EI_CLASS_CURRENT },
			{ EI_DATA, EI_DATA_CURRENT },
			// { EI_OSABI, EI_OSABI_CURRENT } // This is set to 0x3 sometimes (e.g. when compiling with -std=c++17)
		};

		for (size_t i = 0; i < ARRAY_COUNT(verify); i++) {
			if (header.e_ident[verify[i].key] != verify[i].value)
				return false;
		}

		if (header.e_type != ET_DYN)
			return false;
		if (header.e_machine != EM_CURRENT)
			return false;

		Elf_Shdr dynsym = {}, strtab = {};
		if (!findSections(file, header, dynsym, strtab))
			return false;

		// Look for strings in the string table...
		PODArray<Word, 10> offsets(file->engine());
		findNames(file, strtab, name, offsets);
		if (offsets.count() == 0)
			return false;

		// See if any symbols here refer to any of the offsets we found.
		file->seek(dynsym.sh_offset);
		for (size_t i = 0; i*sizeof(Elf_Sym) < dynsym.sh_size; i++) {
			Elf_Sym sym;
			if (!fill(file, sym))
				return false;

			for (Nat i = 0; i < offsets.count(); i++)
				if (offsets[i] == sym.st_name)
					return true;
		}

		return false;
	}

	bool hasExport(Url *file, const char *name) {
		RIStream *src = file->read()->randomAccess();
		bool r = hasExport(src, name);
		src->close();
		return r;
	}

#else
#error "Implement 'loadLibrary', 'unloadLibrary' and 'findLibraryFn'!"
#endif

}
