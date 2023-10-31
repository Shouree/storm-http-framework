#pragma once
#include "Core/Io/Stream.h"
#include "Core/Io/Url.h"
#include "Sound.h"

namespace sound {

	/**
	 * Read sound files. Try to figure out what format is contained by examining the header.
	 */

	// Create a sound stream for reading `src` based on the header in `src`. Will make `src` into a
	// seekable stream to provide a seekable audio stream.
	Sound *STORM_FN sound(IStream *src) ON(Audio);

	// Open sound from a streaming source. The resulting stream will not be seekable.
	Sound *STORM_FN soundStream(IStream *src) ON(Audio);


	// Calls `sound(file.read())`.
	Sound *STORM_FN readSound(Url *file) ON(Audio);

	// Calls `soundStream(file.read())`.
	Sound *STORM_FN readSoundStream(Url *file) ON(Audio);

}
