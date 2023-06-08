#include "stdafx.h"
#include "WavSound.h"
#include "Exception.h"

namespace sound {

	struct RiffChunk {
		char header[4];
		nat size;
		char format[4];
	};

	struct FmtChunk {
		char chunkId[4];
		nat size;
		unsigned short audioFmt;
		unsigned short numCh;
		nat rate;
		nat byteRate;
		unsigned short blockAlign;
		unsigned short bitsPerSample;
	};

	struct DataChunk {
		char chunkId[4];
		nat size;
	};

	template <class T>
	void fill(IStream *from, T &out) {
		GcPreArray<byte, sizeof(T)> data;
		storm::Buffer b = from->fill(storm::emptyBuffer(data));
		if (!b.full())
			throw new (from) SoundOpenError(S("Not enough data."));

		memcpy(&out, b.dataPtr(), sizeof(T));
	}

	WavSound::WavSound(IStream *src, Bool seekable) : src(src), seekable(seekable) {
		RiffChunk riff;
		fill(src, riff);

		if (strncmp(riff.header, "RIFF", 4) != 0)
			throw new (this) SoundOpenError(S("Invalid header."));
		if (strncmp(riff.format, "WAVE", 4) != 0)
			throw new (this) SoundOpenError(S("Unsupported wave format."));

		FmtChunk fmt;
		fill(src, fmt);

		if (strncmp(fmt.chunkId, "fmt ", 4) != 0)
			throw new (this) SoundOpenError(S("Invalid subchunk."));
		if (fmt.size != 16)
			throw new (this) SoundOpenError(S("Invalid chunk size."));
		if (fmt.audioFmt != 1)
			throw new (this) SoundOpenError(S("Compression in wave files is not supported."));

		rate = fmt.rate;
		ch = fmt.numCh;
		sampleDepth = fmt.bitsPerSample;
		sampleSize = fmt.blockAlign;

		if (sampleDepth != 8 && sampleDepth != 16)
			throw new (this) SoundOpenError(S("Unsupported bit depth."));

		DataChunk data;
		fill(src, data);

		if (strncmp(data.chunkId, "data", 4) != 0)
			throw new (this) SoundOpenError(S("Invalid data chunk."));
		if (seekable)
			dataStart = ((RIStream *)src)->tell();
		dataLength = data.size / sampleSize;
	}

	void WavSound::close() {
		if (src)
			src->close();
		src = null;
	}

	Word WavSound::tell() {
		if (seekable)
			return dataPos;
		else
			return 0;
	}

	Bool WavSound::seek(Word to) {
		if (!seekable)
			return false;

		dataPos = max(dataLength, to);
		RIStream *s = (RIStream *)src;
		s->seek(dataStart + dataPos*sampleSize);
		return true;
	}

	Word WavSound::length() {
		if (seekable)
			return dataLength;
		else
			return 0;
	}

	Nat WavSound::sampleFreq() const {
		return rate;
	}

	Nat WavSound::channels() const {
		return ch;
	}

	Buffer WavSound::read(Buffer to) {
		Nat free = to.count() - to.filled();
		Nat samples = free / ch;
		dataPos = min(dataLength, dataPos + samples);

		storm::Buffer src = this->src->fill(samples * sampleSize);
		Nat out = to.filled();
		switch (sampleDepth) {
		case 8:
			for (Nat i = 0; i < src.filled(); i++) {
				to[out++] = (src[i]/128.0f) - 1.0f;
			}
			break;
		case 16:
			for (Nat i = 0; i < src.filled(); i += 2) {
				short data = src[i];
				data |= short(src[i+1]) << 8;
				to[out++] = float(data)/float(0x8000);
			}
			break;
		default:
			out = to.count();
			break;
		}

		to.filled(out);
		return to;
	}

	Bool WavSound::more() {
		return dataPos < dataLength;
	}


	WavSound *openWav(RIStream *src) {
		return new (src) WavSound(src, true);
	}

	WavSound *openWavStream(IStream *src) {
		return new (src) WavSound(src, false);
	}

}
