Files and Streams
=================

This tutorial explores how to handle files and other streams of data using Storm's standard library.
The concepts covered here should be familiar to most programmers that have worked with languages
like C++ and Java, although there are of course some differences.

The code presented in this tutorial is available in the directory `root/tutorials/streams` in the
Storm release. You can run it by typing `tutorials:streams:main` in the Basic Storm interactive
top-loop.

Setup
-----

Before starting to write code, we need somewhere to work. For this tutorial we will create a
directory where we can put our code and some text files that we will work with. Create a directory
somewhere on your system. This tutorial assumes that you name it `streams`. If you wish to use
another name you need to modify the names in the example accordingly.

Create a file `main.bs` inside the directory with the following contents as a starting point:

```bs
use core:io;

void main() {
}
```

After doing this, open a terminal and change to the directory where you created the directory
`streams`. Then run the code by typing:

```
storm streams
```

If done correctly, Storm will exit without output since the `main` function was empty. Note that
based on how you have installed Storm, you might need to [modify the
command-line](md:../Running_Storm/In_the_Terminal) slightly.


The Url Class
-------------

Storm uses the class [stormname:core.io.Url] to represent paths to files in the filesystem (and
generic URL:s). The class represents the path as a protocol and a list of parts. This makes it easy
to inspect and manipulate URLs programmatically.

The `Url` class has a default constructor that creates a representation of a relative path that
refers to the current directory. We can do this as follows:

```bs
use core:io;

void main() {
    Url path;
    print(x.toS);
}
```

Running the program above (using `storm streams`) produces the output `./`, which indicates that the
`Url` is relative (it uses the "relative path" protocol). We can add parts to the `Url` using the
`/` operator as follows:

```bs
use core:io;

void main() {
    Url path;
    path = path / "streams" / "input.txt";
    print(path.toS);
}
```

This program will print `./streams/input.txt`. Again, the `./` at the start of the path simply
indicates that the `Url` represents a relative path.

The `Url` class provides a number of operations for inspecting the path. For example, we can
retrieve the name of the file referred to by the `Url`, with or without the extension:

```bs
use core:io;

void main() {
    Url path;
    path = path / "streams" / "input.txt";
    print("Path: ${path}");
    print("Name: ${path.name}");
    print("Title: ${path.title}");
    print("Extension: ${path.ext}");
    print("Parent: ${path.parent}");

    for (i, x in path) {
        print("Part ${i}: ${x}");
    }
}
```

The code above will print the following:

```
Path: ./streams/input.txt
Name: input.txt
Title: input
Extension: txt
Parent: ./streams/
Part 0: streams
Part 1: input.txt
```

It is worth noting that the `Url` class (or rather the relative protocol) does not support accessing
the file system through relative paths. To actually interact with the file system, we first need to
make the `Url` absolute. This has the additional benefit that the output format and comparisons will
follow the appropriate conventions for the current operating system (e.g. case-insensitive
comparisons on Windows).

To illustrate this, let's try to list the contents of the `streams` directory. We can do this using
the `children` function in the `Url` class:

```bs
use core:io;

void main() {
    Url path = Url() / "streams";
    for (child in path.children()) {
        print("Child: ${child}");
    }
}
```

If we run the code above, we will get the following error since `path` is relative (followed by a
stack-trace):

```
The operation 'children' is not supported by the protocol ./
```

To make `path` absolute, we can either start building `path` from an absolute `Url`, or making it
absolute afterwards. In both of these cases we can use the function `cwdUrl` to retrieve an absolute
`Url` for the current working directory:

```bs
use core:io;

void main() {
    // Either:
    Url path = cwdUrl() / "streams";
    // Or:
    Url path = Url() / "streams";
    path = path.makeAbsolute(cwdUrl());

    for (child in path.children()) {
        print(child.toS);
    }
}
```

With this modification, the program works and prints the name of the `main.bs` file, something like
this:

```
/home/storm/streams/main.bs
```

Let's create some more contents in the `streams` directory to make the output more interesting.
First, we create a directory inside `streams` that we call `res` (you can do this by running
`(cwdUrl() / "streams" / "res").createDir()` in the code). Then we also create a file `example.txt`
inside the `streams` directory with the following content:

```
Lorem ipsum dolor sit amet, consectetur adipisicing elit,
sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
```

If we run the program now, it produces a warning, followed by the following output (the order may be
different on your system):

```
/home/storm/streams/example.txt
/home/storm/streams/main.bs
/home/storm/streams/res/
```

As expected, we can see the file `example.txt` and the directory `res` that we created. It is worth
noting that the directory `res` ends with a `/` to indicate that it is a directory. This information
is stored in the `Url` class, and we can retrieve it using the `dir` member:

```bsstmt
for (child in path.children()) {
    if (child.dir) {
        print("Dir:  ${child}");
    } else {
        print("File: ${child}");
    }
}
```

This change makes the program output the following:

```
File: /home/storm/streams/example.txt
File: /home/storm/streams/main.bs
Dir:  /home/storm/streams/res/
```

Now, let's turn our attention to the warning:

```
WARNING storm::Package::createReaders: No reader for [./streams/example.txt]
(should be lang.txt.reader(core.Array(core.io.Url), core.lang.Package))
```

It is produced since we instructed Storm to load the directory `streams` into the name tree.
However, Storm does not know how to treat files with the extension `.txt`. As such, it informs us
about this, and suggests how we could implement a language that handles `.txt` files.

Because of this, it is usually a good idea to store non-code resources that a program needs in a
separate directory. This places them in a sub-package that is not compiled by default, which avoids
the warning in most cases.

As such, to remove the warning, we simply move `example.txt` into `res`.


Accessing Files in the Name Tree
--------------------------------

When a program requires some data files to function properly (e.g. images), it is convenient to
place these files alongside the code. As mentioned above, it is a good idea to place such files in
their own subdirectory (often called `res`), to avoid the warning from Storm.

The question that remains is, how do we reliably get the path of these files? One approach would be
to create a function like below to get the `Url` to a file in the `res` directory:

```bs
Url resUrl(Str name) {
    return cwdUrl() / "streams" / "res" / name;
}
```

We can then use it as follows in the `main` function:

```bsstmt
Url example = resUrl("example.txt");
print("Does 'example.txt' exist? ${example.exists}");
```

When we run the program, it should produce the following output:

```
Does 'example.txt' exist? true
```

However, since our starting point is the current working directory, it requires that the user starts
the program from the right directory. For example, if we were to do the following, Storm loads the
program successfully, but the program would fail to find the file:

```
cd streams
storm .
```

This still loads the package `streams` properly. However, the program prints:

```
Does 'example.txt' exist? false
```

A more robust way is to ask Storm where the package `res` is located and use that location instead.
We can retrieve the representation of the package used by the compiler using the `named{}` macro in
the package `lang:bs:macro`, and then get the package by calling `url`. However, since not all
packages originate from the file system, this function may return `null`, which we need to handle.
We can implement all of this as follows:

```bs
use core:io;
use lang:bs:macro;

Url resUrl(Str name) {
    if (url = named{res}.url) {
        return url / name;
    } else {
        throw InternalError("Expected the package 'res' to be non-virtual.");
    }
}
```

This version of the `resUrl` function will thus work correctly regardless of what the current
working directory was when the user started Storm.


Reading Files
-------------

Now that we know how to find files in the file system, let's try reading the `example.txt` file we
created before. We can get an input stream [stormname:core.io.IStream] for a file by calling the
`read()` member of the `Url` class:

```bs
void main() {
    Url example = resUrl("example.txt");
    IStream input = example.read();
}
```

After opening the file and getting a stream to the file, we can read from the stream using either
`read` or `fill`. The `read` functions has the same semantics as the `read` function in UNIX. That
is, it is free to read fewer bytes than was requested, even if more data is available. The `fill`
function, on the other hand, guarantees that it fills the buffer with data as long as the end of the
stream is not reached. For this reason, the `fill` function is recommended since it is easier to use
(the `read` function may, however, be necessary in some cases when working with sockets or pipes).

Both functions work with the [stormname:core.io.Buffer] type. It simply represents a sequence of
bytes encapsulated in a convenient container. We can create an empty buffer and ask the stream to
fill it as follows:

```bsstmt
Buffer b = buffer(32);
b = input.fill(b);
print(b.toS);

input.close();
```

Note that even though `Buffer` is a value type, the underlying storage for the buffer is shared
between instances. This means that it is not problematic to create large buffer and passing them
around. The exception to this is, of course, that the buffer is copied whenever it crosses a thread
boundary. This is why `read` and `fill` returns a buffer that is often the same as the one that was
passed to the function: in case a thread switch was necessary, the original buffer will not be
updated, and the one returned from the function has to be used. This is why the re-assignment of `b`
is sometimes important.

The `Buffer` class contains a variable `filled` that indicates how much of the buffer is filled with
data. Note that `filled` is just a marker that other parts of the system use to communicate what
part of the buffer is valid. It is still possible to store data in all parts of the allocated space,
regardless of the value of `filled`.

When we created the buffer with the call to `buffer(32)`, `filled` is zero since the buffer is
initially empty. The `fill` function then fills the buffer with data (starting at `filled`, in case
it is non-zero), and fills as much as possible of the buffer with data as possible. We can see the
result by observing that `filled` is updated to reflect the new state of the buffer. With this
information, we can read the entire contents of the file as follows:

```bsstmt
Buffer b = buffer(32);
do {
    b.filled = 0; // Reset from previous iteration.
    input.fill(b);
    print(b.toS + "\n");
} while (b.filled > 0); // Zero bytes read means end of stream.

input.close();
```

Note that we need to set filled to zero before calling `fill` the second time. Otherwise `fill`
would conclude that the buffer is already full and not read any more data. It is worth noting that
`input` has a member `more` that indicates if more data is available. It usually just keeps track of
whether a `fill` or `read` operation has returned zero bytes previously, so it is just a
convenience on top of the strategy used above.

When using UNIX line endings in the file, the program produces the following output:

```
00000000   4C  6F  72  65  6D  20  69  70  73  75  6D  20  64  6F  6C  6F
00000010   72  20  73  69  74  20  61  6D  65  74  2C  20  63  6F  6E  73
00000020 | 

00000000   65  63  74  65  74  75  72  20  61  64  69  70  69  73  69  63
00000010   69  6E  67  20  65  6C  69  74  2C  0A  73  65  64  20  64  6F
00000020 | 

00000000   20  65  69  75  73  6D  6F  64  20  74  65  6D  70  6F  72  20
00000010   69  6E  63  69  64  69  64  75  6E  74  20  75  74  20  6C  61
00000020 | 

00000000   62  6F  72  65  20  65  74  20  64  6F  6C  6F  72  65  20  6D
00000010   61  67  6E  61  20  61  6C  69  71  75  61  2E  0A| 20  6C  61
00000020   

00000000 | 62  6F  72  65  20  65  74  20  64  6F  6C  6F  72  65  20  6D
00000010   61  67  6E  61  20  61  6C  69  71  75  61  2E  0A  20  6C  61
00000020   
```


The output shows that we needed to run the loop four times to read the entire file. Each output is a
hex-dump of the contents of the `Binary` object. The numbers on the right is the hexadecimal offset
from the start. The remainder of each line are individual bytes in the buffer, in hexadecimal.

The first time, we read 32 bytes successfully. We can see this by observing the location of the `|`
character, that corresponds to the value of `filled`. Since the `|` is at the end of the output, we
managed to fill the buffer entirely. The same is true for the next two buffer outputs. We can also
see that the bytes are different the first three times, even though we re-used the buffer.

The fourth time we can see that something else happened. Here, the `|` is not at the end, but
towards the end of the second line. We can see that the bytes before the `|` were updated, but the
three last bytes are the same as in the previous iteration since they were not overwritten by the
`fill` operation. In fact, since we used `fill`, this observation is enough to conclude that we have
reached the end of the stream. This would not be the case if we had used `read`, since `read` is
allowed to not fill the buffer fully, even if there is more data in the stream.

The last output shows a similar situation, but here `fill` read zero bytes, and therefore the `|` is
before the first byte, and the contents of the buffer is unchanged.


At this point it is worth mentioning that both `read` and `fill` have overloads that creates and
returns a buffer with the specified size (e.g. `input.fill(32)`). They are convenient when reading
data once, but since they allocate new buffers all the time, they may be inefficient when working
with large data.

Finally, it is worth noting that it is usually a good idea to work with larger buffer sizes than 32
bytes. Otherwise, the overhead from accessing the file system tends to be fairly large.


Reading Text
------------

Since the file `example.txt` contains text, we would like to be able to interpret the contents of
the file as text. Luckily, Storm provides [storminfo:core.io.TextInput] streams for this purpose.
Once we have acquired an input stream from a file, we can call `readText` to create a suitable
`TextInput` stream for us. The `readText` function will inspect the first few bytes of the stream,
determine the encoding of the text, and then create a suitable subclass of `TextInput` to handle the
encoding. The `TextInput` subclass also handles conversions of line endings as suitable.

We can do this as follows:

```bs
void main() {
    Url example = resUrl("example.txt");
    IStream input = example.read();
    TextInput text = readText(input); // or input.readText()
    while (text.more()) {
        print("Line: " + text.readLine());
    }
    text.close(); // Also closes 'input'.
}
```

This program prints the contents of the text file we created earlier. The `TextInput` class also
contains the functions `readAll` that reads the entire file into a string if desired.

It is worth noting that the text input stream is buffered. That is, it attempts to read ahead from
its input stream when possible. This is usually not a problem, as the stream will only read more
data if it is readily available, and never wait for additional data to become available. However, it
might cause issues if the `IStream` is used for other purposes in addition to being passed to the
text stream. If it is necessary to extract a part of a stream as text, it is better to store that
portion of the stream in a `Buffer` and use a `MemIStream` as a source for the `TextInput` in order
to ensure that the `TextInput` does not read too many bytes in the input.


Finally, the system contains a convenience function `readAllText` that accepts a `Url` and reads the
entire file as text into a string. As such, we could simplify the entire program above into:

```bsstmt
Str text = resUrl("example.txt").readAllText();
```


Writing Files
-------------

The `Url` class also contains a member `write` that creates a [stormname:core.io.OStream] for a
file. The file is created if it does not already exists, and truncates it if it exists. After
creating the stream, we can use the `write` function to write a buffer to the stream. For example,
the code below will write the character `A` to the file `out.txt`:

```bs
void main() {
    Url out = resUrl("out.txt");
    OStream output = out.write();

    Buffer b(2);
    b[0] = 0x41;
    b[1] = 0x0A;
    b.filled = 2; // Indicate that the buffer is filled with data.

    output.write(b);
    output.close();
}
```

Note that we need to set the buffer's `filled` to 2 in order to tell `write` to write the two bytes
in the buffer. This makes `write` work well alongside `read` and/or `fill` of the `IStream`.

Writing Text
------------

Similarly to reading text, we can use a [storminfo:core.io.TextOutput] class to encode text for us.
However, since it is not possible to automatically detect character encoding in this case, we need
to create the appropriate subclass ourselves.

To encode text into utf-8, we use the [storminfo:core.io.Utf8Output] class. To specify how line
endings and byte-order-marks should be handled, we can pass it a `TextInfo` object that describes
the configuration. It is a good idea to create the `TextInfo` object by calling `sysTextInfo` to get
the default behavior for the current system.

In summary, we can write text to a file as follows:

```bs
void main() {
    Url out = resUrl("out.txt");
    OStream output = out.write();

    Utf8Output textOut(output, sysTextInfo());
    textOut.writeLine("Text from Storm!");
    textOut.writeLine("Another line");

    textOut.close();
}
```

When working with text output, it is worth noting that the text streams are buffered. That is, the
output stream waits until it has gathered a bit of data before writing to the stream. This is
usually not a problem since flushing happens automatically. However, if text streams are used over a
socket or a pipe (e.g. for HTTP), it might be necessary to manually `flush` an output stream.


Standard Streams
----------------

It is possible to access standard input, standard output, and standard error of the Storm process as
streams in Storm. They are available as:

- [stormname:core.io.std.in]
- [stormname:core.io.std.out]
- [stormname:core.io.std.error]

The system also provides text streams for standard input, standard output, and standard error as:

- [stormname:core.stdIn]
- [stormname:core.stdOut]
- [stormname:core.stdError]

Actually, the `print` function is simply implemented as: `stdOut.writeLine(...)`.

Furthermore, it is possible to replace the text streams if desired. This has the effect of
redirecting the output from Storm code to the desired class. This makes it possible to redirect
output to other places programmatically if desired. For example, the language server uses standard
in and standard out to communicate with the editor, and therefore replaces the text streams to be
able to forward them to the editor.
