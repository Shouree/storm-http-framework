#include "stdafx.h"
#include "OggSound.h"
#include "Exception.h"
#include "Utils/Memory.h"
#include <limits>

namespace sound {

	static const GcType vorbisType = {
		GcType::tFixed,
		null,
		null,
		sizeof(OggVorbis_File),
		1,
		{ OFFSET_OF(OggVorbis_File, datasource) }
	};

	static OggVorbis_File *allocFile(Engine &e) {
		return (OggVorbis_File *)runtime::allocStaticRaw(e, &vorbisType);
	}

	OggSound::OggSound(IStream *src, Bool seekable) : src(src), file(null), seekable(seekable), atEnd(false) {
		file = allocFile(src->engine());
		ov_callbacks cb = {
			&OggSound::srcRead,
			&OggSound::srcSeek,
			&OggSound::srcClose,
			&OggSound::srcTell,
		};

		switch (ov_open_callbacks(this, file, null, 0, cb)) {
		case 0:
			// All is well!
			break;
		case OV_ENOTVORBIS:
			throw new (this) SoundOpenError(S("Looks like an OGG file, but it is something else."));
		case OV_EVERSION:
			throw new (this) SoundOpenError(S("Ogg version mismatch."));
		case OV_EBADHEADER:
			throw new (this) SoundOpenError(S("Ogg header corrupt."));
		default:
			throw new (this) SoundOpenError(S("Unknown error when opening OGG file."));
		}

		/**
		 * Note: there seems to be possible to have multiple streams inside a vorbis file, we do not
		 * handle that correctly at the moment. Now, we only show the first stream.
		 */
		vi = ov_info(file, 0);
	}

	OggSound::~OggSound() {
		// If we got this far, we should not attempt to call virtual functions on 'src', as it might crash.
		src = null;

		close();
	}

	void OggSound::close() {
		if (file) {
			ov_clear(file);
			file = null;
		}

		if (src) {
			src->close();
			src = null;
		}
	}

	Bool OggSound::seek(Word to) {
		if (!seekable)
			return false;

		ov_pcm_seek(file, to);
		return true;
	}

	Word OggSound::tell() {
		if (!seekable)
			return 0;
		return ov_pcm_tell(file);
	}

	Word OggSound::length() {
		if (!seekable)
			return 0;
		return ov_pcm_total(file, 0);
	}

	Nat OggSound::sampleFreq() const {
		return vi->rate;
	}

	Nat OggSound::channels() const {
		return vi->channels;
	}

	Buffer OggSound::read(Buffer to) {
		Nat free = to.count() - to.filled();
		Nat ch = channels();
		Nat samples = free / ch;
		float **buffer = null;
		int bitstream = 0;
		long r = ov_read_float(file, &buffer, samples, &bitstream);
		if (r <= 0) {
			atEnd = true;
			samples = 0;
		} else {
			samples = r;
		}

		// Copy...
		nat at = to.filled();
		for (Nat i = 0; i < samples; i++)
			for (Nat c = 0; c < ch; c++)
				to[at++] = buffer[c][i];

		to.filled(at);
		return to;
	}

	Bool OggSound::more() {
		return !atEnd;
	}

	size_t OggSound::srcRead(void *to, size_t size, size_t nmemb, void *data) {
		OggSound *me = (OggSound *)data;
		size_t s = size * nmemb;
		s = min(s, size_t(std::numeric_limits<Nat>::max()));

		storm::Buffer r = me->src->read(Nat(s));
		memcpy(to, r.dataPtr(), r.filled());
		return r.filled();
	}

	int OggSound::srcSeek(void *data, ogg_int64_t offset, int whence) {
		OggSound *me = (OggSound *)data;
		if (!me->seekable)
			return -1;

		RIStream *s = (RIStream *)me->src;
		switch (whence) {
		case SEEK_SET:
			s->seek(offset);
			break;
		case SEEK_CUR:
			s->seek(s->tell() + offset);
			break;
		case SEEK_END:
			s->seek(s->length() + offset);
			break;
		}

		return 0;
	}

	int OggSound::srcClose(void *data) {
		OggSound *me = (OggSound *)data;
		if (me->src)
			me->src->close();
		me->src = null;
		return 0;
	}

	long OggSound::srcTell(void *data) {
		OggSound *me = (OggSound *)data;
		if (!me->seekable)
			return -1;

		RIStream *s = (RIStream *)me->src;
		return (long)s->tell();
	}

	OggSound *openOgg(RIStream *src) {
		return new (src) OggSound(src, true);
	}

	OggSound *openOggStream(IStream *src) {
		return new (src) OggSound(src, false);
	}

}
