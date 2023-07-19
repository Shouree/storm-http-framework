#include "stdafx.h"
#include "Read.h"
#include "OggSound.h"
#include "FlacSound.h"
#include "WavSound.h"
#include "Mp3Sound.h"
#include "Exception.h"

namespace sound {

	// check the header in the stream
	// zeroTerm - Zero-terminated string in the file?
	static bool checkHeader(IStream *file, const char *header, bool zeroTerm) {
		nat len = nat(strlen(header));
		if (zeroTerm)
			len++;

		storm::Buffer buffer = file->peek(storm::buffer(file->engine(), len));
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


	Sound *sound(IStream *src) {
		Sound *result = null;

		if (checkHeader(src, "OggS", false)) {
			result = openOgg(src->randomAccess());
		} else if (checkHeader(src, "fLaC", false)) {
			result = openFlac(src->randomAccess());
		} else if (checkHeader(src, "RIFF", false)) {
			result = openWav(src->randomAccess());
		} else if (checkHeader(src, "ID3", false)) {
			result = openMp3(src->randomAccess());
		} else if (checkHeader(src, "\xFF\xFB", false)) {
			result = openMp3(src->randomAccess());
		} else {
			throw new (src) SoundOpenError(S("Unknown file format."));
		}

		if (!result)
			throw new (src) SoundOpenError(S("Failed to open file."));

		return result;
	}

	Sound *soundStream(IStream *src) {
		Sound *result = null;

		const char *foo = "\xFF\xFB";
		PVAR(strlen("\xFF\xFB"));
		PVAR(int(foo[0]));
		PVAR(int(foo[1]));

		if (checkHeader(src, "OggS", false)) {
			result = openOggStream(src);
		} else if (checkHeader(src, "fLaC", false)) {
			result = openFlac(src->randomAccess());
		} else if (checkHeader(src, "RIFF", false)) {
			result = openWavStream(src);
		} else if (checkHeader(src, "ID3", false)) {
			result = openMp3Stream(src);
		} else if (checkHeader(src, "\xFF\xFB", false)) {
			result = openMp3Stream(src);
		} else {
			throw new (src) SoundOpenError(S("Unknown file format."));
		}

		if (!result)
			throw new (src) SoundOpenError(S("Failed to open file."));

		return result;
	}

	Sound *readSound(Url *file) {
		return sound(file->read());
	}

	Sound *readSoundStream(Url *file) {
		return soundStream(file->read());
	}

}
