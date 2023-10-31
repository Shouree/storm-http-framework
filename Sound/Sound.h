#pragma once
#include "Buffer.h"
#include "Core/TObject.h"

namespace sound {

	/**
	 * Sound source. Something that provides sound for playback.
	 *
	 * All positions are indicated in samples.
	 *
	 * Channels are interleaved as specified in the vorbis format.
	 * For 2 channels: L, R
	 * For 3 channels: L, C, R
	 */
	class Sound : public ObjectOn<Audio> {
		STORM_CLASS;
	public:
		// Create the sound source.
		STORM_CTOR Sound();

		// Close the stream now. Also closes any underlying input streams for this stream.
		virtual void STORM_FN close();

		// Get the current position, measured in samples. May return zero for non-seekable streams.
		virtual Word STORM_FN tell();

		// Seek in the input stream. Returns `false` if seeking is not supported, or an error
		// occurred. The position is measured in samples. The position is expected to be a multiple
		// of the number of channels.
		virtual Bool STORM_FN seek(Word to);

		// Get the length of the stream, measured in samples. Returns zero if the stream is not
		// seekable.
		virtual Word STORM_FN length();

		// Get the sample frequency of the stream, measured in samples per second.
		virtual Nat STORM_FN sampleFreq() const;

		// Get the number of channels for this stream.
		virtual Nat STORM_FN channels() const;

		// Attempt to fill the buffer `to` with samples from the stream. Fills the bytes from
		// `filled` to `count` in the buffer. Only reads a multiple of `channels` samples from the
		// stream. If zero samples are read, then the end of the stream is reached.
		virtual Buffer STORM_FN read(Buffer to);

		// Read data from the buffer into a new, empty buffer with space for the specified number of
		// samples.
		virtual Buffer STORM_FN read(Nat samples);

		// Is more data available in the stream? Might return false only after reading zero bytes
		// for the first time.
		virtual Bool STORM_FN more();

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

}
