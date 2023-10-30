Sound Library
=============

The sound API is based on streams of sound that behave similarly to the
[IO](md:../Standard_Library/IO) part of the standard library. The [stormname:sound.Sound] class is
analogous to a [stormname:core.io.IStream], and the [stormname:sound.Buffer] is analogous to the
[stormname:core.io.Buffer]. The difference is that elements are 32-bit floating point numbers rather
than bytes.

The library also provides support for decoding sound in MP3, OGG, FLAC and WAV formats, and provides
the ability to play sound.

All operations are executed on the [stormname:sound.Audio] thread.


Buffer
------

The [stormname:sound.Buffer] represents a series of samples, represented as 32-bit floating point
values in the range zero to one.



Sound
-----

The [stormname:sound.Sound] represents a stream of sound data...


Player
------

[stormname:sound.Player]


Reading Sound
-------------

```stormdoc
@sound
- .sound(core.io.IStream)
- .soundStream(core.io.IStream)
- .readSound(*)
```
