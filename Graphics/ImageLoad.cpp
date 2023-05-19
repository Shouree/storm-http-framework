#include "stdafx.h"
#include "ImageLoad.h"
#include "Core/Convert.h"

#ifndef WINDOWS

#include <png.h>
#include <jpeglib.h>

namespace graphics {

	/**
	 * Image loading for platforms other than Windows.
	 */


	// IO callbacks from libpng.
	static void pngRead(png_structp png, byte *to, size_t length) {
		IStream *src = (IStream*)png_get_io_ptr(png);
		Buffer out = src->readAll(length);
		if (out.count() > 0)
			memcpy(to, out.dataPtr(), out.count());
	}

	Image *loadPng(IStream *from, const wchar *&error) {
		// Anchor 'from' on the stack so that the GC does not move it.
		IStream *volatile anchor = null;
		atomicWrite(anchor, from);

		png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		png_infop info = null;
		error = S("Failed to initialize libpng");

		// Custom error handling:
		if (setjmp(png_jmpbuf(png))) {
			// Error. Clean up and exit.
			png_destroy_read_struct(&png, &info, NULL);
			error = S("Internal error in libpng.");
			return null;
		}

		png_set_read_fn(png, (void *)anchor, &pngRead);

		if (png) {
			info = png_create_info_struct(png);
			error = S("Failed to create info struct.");
		}

		png_uint_32 ok = 0;
		png_uint_32 width = 0, height = 0;
		int depth = 0, type = 0;
		if (info) {
			png_read_info(png, info);
			ok = png_get_IHDR(png, info, &width, &height, &depth, &type, NULL, NULL, NULL);
			error = S("Failed to read PNG header.");
		}

		Image *output = null;
		if (ok) {
			int typeNoAlpha = type & ~PNG_COLOR_MASK_ALPHA;

			// Fix the format.
			if (typeNoAlpha == PNG_COLOR_TYPE_PALETTE)
				png_set_palette_to_rgb(png);
			if (typeNoAlpha == PNG_COLOR_TYPE_GRAY)
				png_set_gray_to_rgb(png);
			if (png_get_valid(png, info, PNG_INFO_tRNS))
				png_set_tRNS_to_alpha(png);
			if (depth == 16)
				png_set_strip_16(png);
			if (depth < 8) {
				png_color_8p bit;
				if (png_get_sBIT(png, info, &bit))
					png_set_shift(png, bit);
			}
			if (!(type & PNG_COLOR_MASK_ALPHA))
				png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);

			png_read_update_info(png, info);

			// Read the PNG file itself!
			output = new (from) Image(Nat(width), Nat(height));

			// Make sure to pin the internal array by storing a pointer to it on the stack.
			byte *volatile buffer = null;
			atomicWrite(buffer, output->buffer());
			Nat stride = output->stride();

			// Create a GcArray where we can store pointers to inside 'buffer'. Note: it is not
			// scanned, as that would lead to pointers to the middle of objects. Since the buffer is
			// pinned on the stack, it can not move so this should be fine anyway.
			GcArray<byte *> *rows = runtime::allocArray<byte *>(output->engine(), &sizeArrayType, height);
			for (Nat i = 0; i < height; i++)
				rows->v[i] = buffer + stride*i;

			png_read_image(png, rows->v);
		}

		// Clean up any structures we allocated.
		png_destroy_read_struct(&png, &info, NULL);

		return output;
	}

	struct JpegError : jpeg_error_mgr {
		Engine *e;
	};

	void onJpegError(j_common_ptr info) {
		struct JpegError *me = (struct JpegError *)info;

		char buffer[JMSG_LENGTH_MAX];
		(*me->format_message)(info, buffer);

		Str *msg = new (*me->e) Str(toWChar(*me->e, buffer));
		throw new (*me->e) ImageLoadError(msg);
	}

	class JpegInput : public jpeg_source_mgr {
	public:
		JpegInput(IStream *src) : src(src) {
			next_input_byte = NULL;
			bytes_in_buffer = 0;
			init_source = &JpegInput::sInit;
			fill_input_buffer = &JpegInput::sFill;
			skip_input_data = &JpegInput::sSkip;
			resync_to_restart = &jpeg_resync_to_restart;
			term_source = &JpegInput::sClose;
		}

		void init() {
			// Read in 8k chunks.
			current = buffer(src->engine(), 1024*8);
		}

		bool fill() {
			current.filled(0);
			current = src->read(current);

			bytes_in_buffer = current.filled();
			next_input_byte = current.dataPtr();

			return !current.empty();
		}

		void skip(long count) {
			while (size_t(count) > bytes_in_buffer) {
				count -= bytes_in_buffer;
				fill();
			}

			next_input_byte += count;
			bytes_in_buffer -= count;
		}

	private:
		// Source.
		IStream *src;

		// Current buffer.
		Buffer current;

		// Wrapper functions.
		static void sInit(j_decompress_ptr info) {
			((JpegInput *)info->src)->init();
		}
		static boolean sFill(j_decompress_ptr info) {
			return ((JpegInput *)info->src)->fill() ? TRUE : FALSE;
		}
		static void sSkip(j_decompress_ptr info, long count) {
			((JpegInput *)info->src)->skip(count);
		}
		static void sClose(j_decompress_ptr info) {
			// Usually nothing is required here. Called when reading an image is finished.
		}
	};

	Image *loadJpeg(IStream *from, const wchar *&error) {
		struct jpeg_decompress_struct decode;
		JpegError errorMgr;
		errorMgr.e = &from->engine();
		decode.err = jpeg_std_error(&errorMgr);
		decode.err->error_exit = &onJpegError;

		try {
			jpeg_create_decompress(&decode);
			JpegInput input(from);
			decode.src = &input;

			if (jpeg_read_header(&decode, TRUE) != JPEG_HEADER_OK)
				throw new (from) ImageLoadError(S("No JPEG header was found."));

			// Set up output format and start decompression.
			decode.out_color_space = JCS_EXT_RGBA;
			jpeg_start_decompress(&decode);

			// Create and fill target bitmap.
			Image *out = new (from) Image(decode.output_width, decode.output_height);
			while (decode.output_scanline < decode.output_height) {
				byte *buf = out->buffer() + out->stride()*decode.output_scanline;
				jpeg_read_scanlines(&decode, &buf, 1);
			}

			// Clean up.
			jpeg_finish_decompress(&decode);
			jpeg_destroy_decompress(&decode);
			return out;
		} catch (...) {
			// Clean up!
			jpeg_destroy_decompress(&decode);
			throw;
		}
	}

}

#endif
