Sound
======

The sound API is based around streams of sound data (the `Sound` class). Each stream provides data
in 32-bit float format, 1 or more channels at some sample frequency. These streams can be chained
together and combined using custom filters that acts as source streams themselves.

To play sound, use the `Player` class. A `Player` will play one sound stream to the standard sound
device in the system. The `Player` class also allows basic playback controls of the stream.

Currently, decoding of `mp3`, `ogg`, `flac` and `wav` files are supported. The function `sound` or
`streamingSound` can be used to automatically detect the format of a stream and create an
appropriate sound stream.