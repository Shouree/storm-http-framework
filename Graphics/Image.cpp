#include "stdafx.h"
#include "Image.h"
#include "ImageLoad.h"
#include "BmpLoad.h"
#include "PPMLoad.h"

namespace graphics {

	// check the header in the stream
	// zeroTerm - Zero-terminated string in the file?
	bool checkHeader(storm::IStream *file, const char *header, bool zeroTerm) {
		nat len = nat(strlen(header));
		if (zeroTerm)
			len++;

		Buffer buffer = file->peek(storm::buffer(file->engine(), len));
		if (!buffer.full()) {
			return false;
		}

		for (nat i = 0; i < len; i++) {
			if (byte(buffer[i]) != byte(header[i])) {
				return false;
			}
		}

		return true;
	}

	Image *loadImage(Url *file) {
		if (!file->exists()) {
			Str *msg = TO_S(file, S("The file ") << file << S(" does not exist."));
			throw new (file) ImageLoadError(msg);
		}
		return loadImage(file->read());
	}

	Image *loadImage(storm::IStream *from) {
		Image *loaded = null;
		const wchar *error = S("The image file type was not recognized.");

		if (checkHeader(from, "\xff\xd8", false)) {
			// JPEG file
			loaded = loadJpeg(from, error);
		} else if (checkHeader(from, "JFIF", false)) {
			// JFIF file
			loaded = loadJpeg(from, error);
		} else if (checkHeader(from, "Exif", false)) {
			// Exif file
			loaded = loadJpeg(from, error);
		} else if (checkHeader(from, "\x89PNG", false)) {
			// PNG file
			loaded = loadPng(from, error);
		} else if (checkHeader(from, "BM", false)) {
			// BMP file
			loaded = loadBmp(from, error);
		} else if (isPPM(from)) {
			// PPM file.
			loaded = loadPPM(from, error);
		}

		if (!loaded)
			throw new (from) ImageLoadError(error);

		return loaded;
	}
}
