Sound Library
=============

The sound library is an external library for managing audio. It provides an interface for working
with streams of sound data that is similar to the [IO](md:../Standard_Library/IO) part of the
standard library (but works on 32-bit floating point values instead of bytes). It also provides
decoding of common audio formats (currently, mp3, ogg, flac, and wav), and playback of audio
streams. The library also defines a thread, [stormname:sound.Audio], for executing sound-related
code.

The remainder of this part of the manual provides an overview of these interfaces.

Sound
-----

The type [stormname:sound.Sound] represents a stream of audio samples. As such, it is analogous to
the type [stormname:core.io.IStream] in the standard library. The type itself does not provide any
audio data. Any of the derived types may be used for this purpose.

A sound stream is considered to consist of a sequence of *samples*. Each sample is represented by a
32-bit floating point number ([stormname:core.Float]). If the sound stream contains multiple
*channels*, samples for all channels are interleaved in the stream as specified by the *vorbis*
format. For example, for 2 channels, the first sample is for the left channel, the second is for the
right, and so on. The interface expects consumers of the sound data to work in multiples of the
channel size.

Audio streams may be seekable or non-seekable. Seekable streams return a non-zero length, while
non-seekable streams return a zero length.

The type has the following members:

```stormdoc
@sound.Sound
- .sampleFreq()
- .channels()
- .close()
- .more()
- .length()
- .seek(core.Word)
- .tell()
- .read(*)
```

### Reading Sound

As mentioned above, the library contains support for some common audio formats, namely mp3, ogg,
flac, and wav. Support for most of these are provided by external libraries that are compiled into
the library on most platforms. Support for wav is native to the library, and as such support for
compressed wav-files is limited.

The library provides the following classes to decode the different formats:

- [stormname:sound.Mp3Sound]
- [stormname:sound.OggSound]
- [stormname:sound.FlacSound]
- [stormname:sound.WavSound]

These are generally created by the functions `openX` or `openXStream`. The former accepts a
[stormname:core.io.RIStream] and creates a seekable stream, while the latter accepts a
[stormname:core.io.IStream] and creates a non-seekable stream.

The library also provides generic operations for opening sound files and automatically deducing the
file format based on the header in the file:

```stormdoc
- sound.sound(core.io.IStream)
- sound.soundStream(core.io.RIStream)
- sound.readSound(core.io.Url)
- sound.readSoundStream(core.io.Url)
```


Buffer
------

The [stormname:sound.Buffer] represents a series of samples, represented as 32-bit floating point
values in the range 1.0 to -1.0. The interface closely resembles that of [stormname:core.io.Buffer],
which is documented [here](md:../Standard_Library/IO/Buffers).

As with the buffers in the IO library, copies of buffers generally refer to the same data, but are
copied across thread boundaries. As such, when using the `read` function in a `Sound` stream, it is
important to use the returned buffer, even though the buffer that was passed to the function is
updated in most cases.

The `Buffer` has the following members:

```stormdoc
@sound.Buffer
- .__init()
- .count()
- .filled()
- .filled(Nat)
- .free()
- .[](*)
- .empty()
- .full()
- .toS(core.StrBuf)
- .outputMark(core.StrBuf, core.Nat)
```

The buffer also has the following free function `buffer` to create buffers:

```stormdoc
- sound.buffer(core.Nat)
```


Player
------

The type [stormname:sound.Player] represents an audio stream that is being played on the system's
default speakers. On Windows, it uses the DirectSound API, and on Linux it relies on OpenAL. The
implementation typicallby buffers some amount of sound data internally to avoid artifacts. The
`Player` provides basic controls for the audio playback:

```stormdoc
@sound.Player
- .__init(sound.Sound)
- .close()
- .play()
- .playing()
- .pause()
- .stop()
- .time()
- .volume()
- .volume(core.Float)
- .wait()
- .waitUntil(*)
```

