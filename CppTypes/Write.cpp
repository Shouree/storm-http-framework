#include "stdafx.h"
#include "Write.h"
#include "Exception.h"
#include "World.h"
#include "Utils/FileStream.h"
#include "Utils/MemoryStream.h"
#include "Utils/TextReader.h"

static String lineEnds(TextReader *src) {
	String result = L"\n";
	nat pos = src->position();

	wchar prev = 0;
	while (src->more()) {
		wchar c = src->get();
		if (prev == '\r' && c == '\n') {
			result = L"\r\n";
			break;
		} else if (c == '\n') {
			result = L"\n";
			break;
		}
		prev = c;
	}

	src->seek(pos);
	return result;
}

static void generatePart(TextWriter *to, const String &indent, const String &endl, GenerateFn fn, World &world) {
	std::wostringstream tmp;
	(*fn)(tmp, world);

	String cont = tmp.str();
	bool indented = false;
	for (nat i = 0; i < cont.size(); i++) {
		if (cont[i] == '\n') {
			to->put(endl);
			indented = false;
		} else {
			if (!indented) {
				// Do not indent preprocessor directives!
				if (cont[i] != '#')
					to->put(indent);
				indented = true;
			}
			to->put(cont[i]);
		}
	}
}

static String findIndentation(const String &line) {
	for (nat i = 0; i < line.size(); i++) {
		switch (line[i]) {
		case ' ':
		case '\t':
			break;
		default:
			return line.substr(0, i);
		}
	}
	return L"";
}

void generateFile(const Path &src, const Path &dest, const GenerateMap &actions, World &world) {
	{
		Path folder = dest.parent();
		if (!folder.exists())
			folder.createDir();
	}

	FileStream *rStream = new FileStream(src, Stream::mRead);
	if (!rStream->valid()) {
		delete rStream;
		throw Error(L"Failed to open the template file: " + toS(src), SrcPos());
	}
	TextReader *read = TextReader::create(rStream);
	TextWriter *write = TextWriter::create(new FileStream(dest, Stream::mWrite), true, read->format());
	String endl = lineEnds(read);

	try {
		while (read->more()) {
			String line = read->getLine();

			write->put(line);
			write->put(endl);

			size_t pos = line.find(L"// ");
			if (pos != String::npos) {
				String part = line.substr(pos + 3);
				String indent = findIndentation(line);

				GenerateMap::const_iterator i = actions.find(part);
				if (i != actions.end()) {
					generatePart(write, indent, endl, i->second, world);
				}
			}
		}

		delete read;
		delete write;
	} catch (...) {
		delete read;
		delete write;

		// Make sure we re-run later!
		dest.deleteFile();

		throw;
	}
}

void generateDoc(const Path &out, World &world) {
	{
		Path folder = out.parent();
		if (!folder.exists())
			folder.createDir();
	}


	// Index part of the file. Contains character offsets for all entries. For entry 'n', the index
	// 'n - 1' and 'n' tells the beginning and end of the actual data.
	const vector<Auto<Doc>> src = world.documentation;
	vector<nat> index(src.size() + 1, 0);

	// Write all data to memory first.
	MemoryStream data;
	{
		textfile::Utf8Writer w(&data, false, false);
		for (nat i = 0; i < src.size(); i++) {
			w.put(src[i]->v);
			index[i + 1] = nat(data.pos());
		}
	}

	// Offset the indexes to make room for the table itself.
	for (nat i = 0; i < index.size(); i++)
		index[i] += nat(sizeof(nat)*index.size());

	// Write everything to file.
	FileStream file(out, Stream::mWrite);
	file.write(nat(sizeof(nat)*index.size()), &index[0]);
	file.write(&data);
}
