#include "stdafx.h"
#include "Mp3Sound.h"
#include "Exception.h"
#include "Core/Convert.h"
#include "Utils/Memory.h"
#include <limits>

namespace sound {

	/**
	 * Allocated in a non-moving pool so that we can pass the handle to mpg123.
	 */
	struct Mp3Stream {
		IStream *src;
		Bool seekable;
	};

	/**
	 * Type description for Mp3Stream above.
	 */
	static storm::GcType mp3StreamType = {
		GcType::tFixed,
		null, // type
		null, // finalizer
		sizeof(Mp3Stream), // stride
		1, // # of offsets
		{ OFFSET_OF(Mp3Stream, src) }, // Offset to 'stream'.
	};


	Mp3Sound::Mp3Sound(IStream *src, Bool seekable) : rate(0), ch(0), h(null), stream(null), atEnd(false) {
		// TODO: Pick dithering decoder?
		h = mpg123_new(NULL, NULL);

		int flags = MPG123_FORCE_FLOAT;
		// Note: we can use MPG123_PICTURE to load ID3v2 images.
		if (seekable)
			flags |= MPG123_FORCE_SEEKABLE;
		else
			flags |= MPG123_NO_PEEK_END;
		mpg123_param(h, MPG123_FLAGS, flags, 0);

		stream = (Mp3Stream *)runtime::allocStaticRaw(engine(), &mp3StreamType);
		stream->src = src;
		stream->seekable = seekable;

		mpg123_replace_reader_handle(h, &Mp3Sound::read, &Mp3Sound::seek, &Mp3Sound::cleanup);
		int ok = mpg123_open_handle(h, stream);

		long rate = 0;
		int channels, encoding = 0;
		if (ok == MPG123_OK) {
			ok = mpg123_getformat(h, &rate, &channels, &encoding);
		}

		if (ok == MPG123_OK) {
			// It seems mpg123 could indicate 'stereo and mono' using 'channels == 3'.
			if (channels > 2)
				channels = 2;

			this->rate = rate;
			this->ch = channels;

			if (encoding != MPG123_ENC_FLOAT_32) {
				mpg123_delete(h);
				h = null;

				throw new (this) SoundOpenError(S("Expected float output from mpg123."));
			}
		} else {
			Str *msg = new (this) Str(toWChar(engine(), mpg123_strerror(h)));
			mpg123_delete(h);
			h = null;

			throw new (this) SoundOpenError(msg);
		}
	}

	Mp3Sound::~Mp3Sound() {
		// Do not try to close the stream. That might already have been done.
		stream = null;

		close();
	}

	void Mp3Sound::close() {
		// Close MP3 decoder.
		if (h) {
			mpg123_delete(h);
			h = null;
		}

		if (stream && stream->src) {
			stream->src->close();
			stream->src = null;
			stream = null;
		}
	}

	Word Mp3Sound::tell() {
		if (stream && stream->seekable) {
			return Word(mpg123_tell(h));
		}
		return 0;
	}

	Bool Mp3Sound::seek(Word to) {
		if (stream && stream->seekable) {
			atEnd = false;
			return mpg123_seek(h, off_t(to), SEEK_SET) >= 0;
		}
		return false;
	}

	Word Mp3Sound::length() {
		if (stream && stream->seekable) {
			off_t l = mpg123_length(h);
			if (l == MPG123_ERR) {
				// Try calling mpg123_scan to find the length.
				mpg123_scan(h);
				l = mpg123_length(h);
			}

			if (l == MPG123_ERR)
				return 0;
			return Word(l);
		}
		return 0;
	}

	Nat Mp3Sound::sampleFreq() const {
		return rate;
	}

	Nat Mp3Sound::channels() const {
		return ch;
	}

	Buffer Mp3Sound::read(Buffer to) {
		if (atEnd)
			return to;

		Nat free = to.count() - to.filled();
		size_t filled = 0;
		int ok = mpg123_read(h, (byte *)(to.dataPtr() + to.filled()), free * sizeof(float), &filled);
		if (ok != MPG123_OK) {
			switch (ok) {
			case MPG123_NEW_FORMAT:
				WARNING(L"Multi-format files not supported.");
				break;
			case MPG123_DONE:
				break;
			default:
				WARNING(mpg123_strerror(h));
				break;
			}
			atEnd = true;
		}

		if (filled == 0)
			atEnd = true;

		to.filled(to.filled() + filled/sizeof(float));
		return to;
	}

	Bool Mp3Sound::more() {
		return !atEnd;
	}

	ssize_t Mp3Sound::read(void *handle, void *to, size_t count) {
		Mp3Stream *stream = (Mp3Stream *)handle;
		storm::Buffer b = stream->src->read(Nat(min(std::numeric_limits<Nat>::max(), count)));
		memcpy(to, b.dataPtr(), b.filled());
		return b.filled();
	}

	off_t Mp3Sound::seek(void *handle, off_t offset, int mode) {
		Mp3Stream *stream = (Mp3Stream *)handle;
		if (!stream->seekable)
			return -1;

		// We know this is possible since we checked 'seekable'.
		RIStream *src = (RIStream *)stream->src;

		switch (mode) {
		case SEEK_SET:
			src->seek(Word(offset));
			break;
		case SEEK_CUR:
			src->seek(Word(src->tell() + offset));
			break;
		case SEEK_END:
			src->seek(Word(src->length() + offset));
			break;
		default:
			return -1;
		}

		return off_t(src->tell());
	}

	void Mp3Sound::cleanup(void *handle) {
		Mp3Stream *stream = (Mp3Stream *)handle;
		if (stream->src)
			stream->src->close();
	}

	Mp3Sound *openMp3(RIStream *src) {
		return new (src) Mp3Sound(src, true);
	}

	Mp3Sound *openMp3Stream(IStream *src) {
		return new (src) Mp3Sound(src, false);
	}

}
