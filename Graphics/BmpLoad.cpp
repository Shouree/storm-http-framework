#include "stdafx.h"
#include "BmpLoad.h"
#include "Utils/Bitwise.h"

namespace graphics {

	/**
	 * Structs used when parsing a Windows BMP/DIB file.
	 *
	 * NOTE: Numbers are stored in little endian format, which works fine as long as we are running
	 * on an X86 cpu.
	 */

	/**
	 * Bitmap header, excluding the 2-byte type field containing "BM", since that would make the
	 * rest of the fields misaligned.
	 */
	struct FileHeader {
		// File size in bytes.
		nat size;

		// Reserved. Should be 0.
		nat reserved;

		// Offset to start of pixel data.
		nat pixelOffset;
	};


	/**
	 * Description of the image. Located directly after FileHeader.
	 */
	struct ImageHeader {
		// Header size in bytes. May be larger than this header.
		nat size;

		// Image width in pixels.
		nat width;

		// Image height in pixels.
		nat height;

		// Number of planes. Must be 1.
		nat16 planes;

		// Number of bits per pixel. 1, 4, 8, 16, 24, or 32.
		nat16 pixelDepth;

		// Compression type (0 = not compressed).
		nat compression;

		// Image size in bytes. Possibly zero for uncompressed images.
		nat imageSize;

		// Resolution in pixels per meter.
		nat xResolution;
		nat yResolution;

		// Number of color map entries used.
		nat colorsUsed;

		// Number of significant colors.
		nat colorsImportant;
	};


	/**
	 * An entry in the color table. Located after ImageHeader.
	 */
	struct ImageColor {
		byte b;
		byte g;
		byte r;
		byte pad;
	};


	/**
	 * Helper for decoding bitfields.
	 */
	class Bitfield {
	public:
		explicit Bitfield(Nat mask) : shift(0), scale(0), mask(mask) {
			if (mask == 0)
				return;

			// Put the mask aligned at bit 0.
			for (Nat tmp = mask; (tmp & 0x1) == 0; tmp >>= 1)
				shift++;

			// Find where the mask ends.
			Nat width = 0;
			for (Nat tmp = mask >> shift; tmp; tmp >>= 1)
				width++;

			// Compute 'mask'.
			for (Nat bits = 0; bits < 8; bits += width)
				scale = (scale << width) | 0x1;

			// Fix 'mask' so that it is properly aligned to the final 16-bit shift.
			scale <<= (16 + 8) - roundUp(Nat(8), width);
		}

		Byte decode(Nat value) {
			if (mask == 0)
				return 255;

			value &= mask;
			value >>= shift;
			value *= scale;
			value >>= 16;
			return Byte(value);
		}

	private:
		Nat shift;
		Nat scale;
		Nat mask;
	};


	// Fill a structure with data from a stream.
	template <class T>
	static bool fill(IStream *src, T &out) {
		GcPreArray<byte, sizeof(T)> data;
		Buffer r = src->readAll(emptyBuffer(data));
		if (r.filled() != sizeof(T))
			return false;
		memcpy(&out, r.dataPtr(), sizeof(T));
		return true;
	}

	// Read data to an array.
	template <class T>
	T *read(IStream *src, Nat count) {
		Nat size = count*sizeof(T);
		Buffer r = src->readAll(buffer(src->engine(), size));
		if (r.filled() != size)
			return null;
		return (T *)r.dataPtr();
	}

	// Decode various bit depths.
	static bool decode1(IStream *from, Image *to, const ImageHeader &header, Nat untilStart);
	static bool decode4(IStream *from, Image *to, const ImageHeader &header, Nat untilStart);
	static bool decode8(IStream *from, Image *to, const ImageHeader &header, Nat untilStart);
	static bool decode16(IStream *from, Image *to, const ImageHeader &header, Nat untilStart);
	static bool decode24(IStream *from, Image *to, const ImageHeader &header, Nat untilStart);
	static bool decode32(IStream *from, Image *to, const ImageHeader &header, Nat untilStart);

	// Choose a good decoder.
	typedef bool (*Decoder)(IStream *from, Image *to, const ImageHeader &header, Nat untilStart);
	static Decoder pickDecoder(const ImageHeader &header);

	Image *loadBmp(IStream *from, const wchar *&error) {
		// Keep track of file offset.
		Nat position = 0;

		{
			Buffer h = from->readAll(2);
			error = S("Invalid BMP header.");
			if (h.filled() != 2)
				return null;
			if (h[0] != 'B' || h[1] != 'M')
				return null;

			position += 2;
		}

		// Read the rest of the header.
		FileHeader header;
		error = S("Invalid or incomplete BMP header.");
		if (!fill(from, header))
			return null;
		position += sizeof(FileHeader);

		ImageHeader image;
		if (!fill(from, image))
			return null;
		position += sizeof(ImageHeader);
		if (image.size < 40)
			return null;
		if (image.planes != 1)
			return null;

		Decoder decode = pickDecoder(image);
		error = S("Unsupported bit depth in the image.");
		if (!decode)
			return null;

		// Now, we can create the output image and start writing to it.
		Image *result = new (from) Image(image.width, image.height);
		error = S("Failed reading the image.");

		Nat remaining = header.pixelOffset - position;
		if ((*decode)(from, result, image, remaining))
			return result;

		return null;
	}

	// Pick a decoder.
	static Decoder pickDecoder(const ImageHeader &header) {
		switch (header.pixelDepth) {
		case 1:
			if (header.compression != 0)
				return null;
			return &decode1;
		case 4:
			if (header.compression != 0)
				return null;
			return &decode4;
		case 8:
			if (header.compression != 0)
				return null;
			return &decode8;
		case 16:
			if (header.compression != 3)
				return null;
			return &decode16;
		case 24:
			if (header.compression != 0)
				return null;
			return &decode24;
		case 32:
			if (header.compression != 3)
				return null;
			return &decode32;
		}

		return null;
	}

	static bool decode32(IStream *from, Image *to, const ImageHeader &header, Nat untilStart) {
		Nat w = to->width();
		Nat h = to->height();

		// Read bitfields.
		Nat r = 0, g = 0, b = 0, a = 0;
		if (!fill(from, r) || !fill(from, g) || !fill(from, b))
			return false;

		untilStart -= 3*sizeof(Nat);

		// Is there an alpha channel?
		if (header.size >= sizeof(ImageHeader) + 3*sizeof(Nat)) {
			// Probably, yes.
			if (!fill(from, a))
				return false;
			untilStart -= sizeof(Nat);
		}

		Bitfield rBit(r);
		Bitfield gBit(g);
		Bitfield bBit(b);
		Bitfield aBit(a);
		from->readAll(untilStart);

		Nat stride = w*4;
		Buffer src = buffer(from->engine(), stride);
		for (Nat y = 0; y < h; y++) {
			src.filled(0);
			src = from->read(src);
			if (src.filled() != stride)
				return false;

			byte *dest = to->buffer(0, h - y - 1);
			for (Nat x = 0; x < w; x++) {
				Nat px = src[x*4 + 0];
				px |= Nat(src[x*4 + 1]) << 8;
				px |= Nat(src[x*4 + 2]) << 16;
				px |= Nat(src[x*4 + 3]) << 24;

				dest[4*x + 0] = rBit.decode(px);
				dest[4*x + 1] = gBit.decode(px);
				dest[4*x + 2] = bBit.decode(px);
				dest[4*x + 3] = aBit.decode(px);
			}
		}

		return true;
	}

	static bool decode24(IStream *from, Image *to, const ImageHeader &header, Nat untilStart) {
		Nat w = to->width();
		Nat h = to->height();

		// Skip until the start of the file.
		from->readAll(untilStart);

		Nat stride = roundUp(w*3, Nat(4));
		Buffer src = buffer(from->engine(), stride);
		for (Nat y = 0; y < h; y++) {
			src.filled(0);
			src = from->read(src);
			if (src.filled() != stride)
				return false;

			byte *dest = to->buffer(0, h - y - 1);
			for (Nat x = 0; x < w; x++) {
				dest[4*x + 0] = src[3*x + 2];
				dest[4*x + 1] = src[3*x + 1];
				dest[4*x + 2] = src[3*x + 0];
				dest[4*x + 3] = 255;
			}
		}

		return true;
	}

	static bool decode16(IStream *from, Image *to, const ImageHeader &header, Nat untilStart) {
		Nat w = to->width();
		Nat h = to->height();

		// Read bitfields.
		Nat r = 0, g = 0, b = 0, a = 0;
		if (!fill(from, r) || !fill(from, g) || !fill(from, b))
			return false;

		// Is there an alpha channel?
		if (header.size >= sizeof(ImageHeader) + 3*sizeof(Nat)) {
			// Probably, yes.
			if (!fill(from, a))
				return false;
			untilStart -= sizeof(Nat);
		}

		Bitfield rBit(r);
		Bitfield gBit(g);
		Bitfield bBit(b);
		Bitfield aBit(a);

		untilStart -= 3*sizeof(Nat);
		from->readAll(untilStart);

		Nat stride = roundUp(w*2, Nat(4));
		Buffer src = buffer(from->engine(), stride);
		for (Nat y = 0; y < h; y++) {
			src.filled(0);
			src = from->readAll(src);
			if (src.filled() != stride)
				return false;

			byte *dest = to->buffer(0, h - y - 1);
			for (Nat x = 0; x < w; x++) {
				Nat px = src[x*2 + 0];
				px |= Nat(src[x*2 + 1]) << 8;

				dest[4*x + 0] = rBit.decode(px);
				dest[4*x + 1] = gBit.decode(px);
				dest[4*x + 2] = bBit.decode(px);
				dest[4*x + 3] = aBit.decode(px);
			}
		}

		return true;
	}

	static bool decode8(IStream *from, Image *to, const ImageHeader &header, Nat untilStart) {
		Nat w = to->width();
		Nat h = to->height();

		Nat used = header.colorsUsed;
		if (used == 0)
			used = 256;

		ImageColor *palette = read<ImageColor>(from, used);
		if (!palette)
			return false;

		untilStart -= used * sizeof(ImageColor);
		from->readAll(untilStart);

		Nat stride = roundUp(w, Nat(4));
		Buffer src = buffer(from->engine(), stride);
		for (Nat y = 0; y < h; y++) {
			src.filled(0);
			src = from->readAll(src);
			if (src.filled() != stride)
				return false;

			byte *dest = to->buffer(0, h - y - 1);
			for (Nat x = 0; x < w; x++) {
				byte color = src[x];
				if (color >= used)
					color = used; // Pick a color so that we do not crash.
				ImageColor *c = &palette[color];

				dest[4*x + 0] = c->r;
				dest[4*x + 1] = c->g;
				dest[4*x + 2] = c->b;
				dest[4*x + 3] = 255;
			}
		}

		return true;
	}

	static bool decode4(IStream *from, Image *to, const ImageHeader &header, Nat untilStart) {
		Nat w = to->width();
		Nat h = to->height();

		Nat used = header.colorsUsed;
		if (used == 0)
			used = 16;

		ImageColor *palette = read<ImageColor>(from, used);
		if (!palette)
			return false;

		untilStart -= used * sizeof(ImageColor);
		from->readAll(untilStart);

		Nat stride = roundUp((w + 1)/2, Nat(4));
		Buffer src = buffer(from->engine(), stride);
		for (Nat y = 0; y < h; y++) {
			src.filled(0);
			src = from->readAll(src);
			if (src.filled() != stride)
				return false;

			byte *dest = to->buffer(0, h - y - 1);
			for (Nat x = 0; x < w; x++) {
				byte color = src[x / 2];
				color = (color >> (~x & 0x1)*4) & 0xF;
				if (color >= used)
					color = used; // Pick a color so that we do not crash.
				ImageColor *c = &palette[color];

				dest[4*x + 0] = c->r;
				dest[4*x + 1] = c->g;
				dest[4*x + 2] = c->b;
				dest[4*x + 3] = 255;
			}
		}

		return true;
	}

	static bool decode1(IStream *from, Image *to, const ImageHeader &header, Nat untilStart) {
		Nat w = to->width();
		Nat h = to->height();

		from->readAll(untilStart);

		Nat stride = roundUp((w + 7)/8, Nat(4));
		Buffer src = buffer(from->engine(), stride);
		for (Nat y = 0; y < h; y++) {
			src.filled(0);
			src = from->readAll(src);
			if (src.filled() != stride)
				return false;

			byte *dest = to->buffer(0, h - y - 1);
			for (Nat x = 0; x < w; x++) {
				byte color = src[x / 8];
				color = (color >> (7 - (x & 0x7))) & 0x1;

				dest[4*x + 0] = color * 255;
				dest[4*x + 1] = color * 255;
				dest[4*x + 2] = color * 255;
				dest[4*x + 3] = 255;
			}
		}

		return true;
	}

}
