Serialization
==============

Storm has a library that provides serialization facilities for objects. These facilities are
implemented by the classes `core.io.ObjIStream` and `core.io.ObjOStream`, which specify the binary
format and provide the low-level operations required to implement serialization. These classes are
complemented by a serialization library written in Basic Storm, that generates the members `read`
and `write` for the classes that should be serialized.

The format used for serialization contains enough information to inspect it without having access to
the original type information; the needed type information is stored inside the serialization
stream. The class `util.serialize.TextObjStream` can be used to inspect and pretty-print previously
serialized objects.

The serialization format is designed to allow serializing multiple objects inside a single stream
while minimizing overhead. Therefore, type information is only sent once in one session (represented
by an instance of `ObjIStream` or `ObjOStream` respectively), and thus care needs to be taken to
ensure that the encoder and decoder use the same length of sessions. For example, if the encoder
writes two objects using one instance of `ObjOStream`, but the reader reads the two objects using
different instances of `ObjIStream`, deserialization will most likely fail.


Serializing data
------------------

Using the serialization is fairly easy. All serializable types provide a `write` and a `read`
function that handles serialization and deserialization respectively. They shall have the following
signatures (`T` is the type being serialized):

- `void write(T this, ObjOStream to)`: The `write` function is a member function of `T`. It writes the
  entire object recursively to the stream.
- `T read(ObjIStream from)` : The `read` function is a static member function of `T` that reads an
  instance of `T` from the stream and returns the created instance.

Both `ObjOStream` and `ObjIStream` have a constructor that takes a single `IStream` or `OStream`,
and they are thus easily created. As mentioned above, these object streams keep track of state
associated with the serialization in order to not duplicate data in the stream, and to keep the
serialized data stream intact. Therefore, if multiple objects are written using one `OStream`
instance, they have to be read back using one `IStream` instance. Conversely, if multiple objects
are written using multiple `OStream` instances, they have to be read back using multiple `IStream`
instances.

The serialization handles inheritance, shared instances and cyclic structures, just like the clone
mechanism does. This means an object that contains (directly or indirectly) two references to the
same object will be serialized and deserialized just like expected, each distinct object is
serialized once, and all references that were originally referring to the same object will still
refer to the same object in the deserialized object. The same is true for structures containing
cycles. Each object serialized at the top-level (i.e. the first call to `write` or `read`) are
considered separate entities in this regard, meaning that two top-level objects will not share any
data with each other, even if they have some objects in common. This means that reading two objects
from a single `ObjIStream` at top-level guarantees that the two object hierarchies are disjoint
(both of them are, of course, also disjoint with the original, serialized object, which may no
longer exist).

As mentioned earlier, it is possible to inspect serialized data in textual format using the class
`util.serialize.TextObjStream`. This object works much like `ObjIStream`, but instead of producing
objects, it produces human readable text. For this stream, just like with `ObjIStream`, it is
important to match the number of instances when reading with the number of instances when writing.


Limiting impact of untrusted data
-----------------------------------

To provide basic protection against deserialization of untrusted data, it is possible to impose
various limits on the size of the deserialized data. The `ObjIStream` provides the following
variables for this purpose:

- `maxReadSize`: Limits the total size of each object hierarchy that is read through a top-level
  call to `read`. This number accounts for all objects and arrays created by the deserialized
  data. However, it does not account for all forms of overhead (e.g. object headers, extra memory
  used for hash tables, etc.). As such, do not assume that the limit corresponds to an exact number
  of bytes.
- `maxArraySize`: Limits the number of array allocations. Array allocations could be particularly
  problematic since ambiguous references causes the garbage collector to retain a small number of
  objects that could be reclaimed. Large array allocations are more likely to be pinned in this
  manner, and the impact of retaining them are quite significant. This is why the array size can be
  controlled separately.
- `maxTypeDescSize`: Limits the size of the data retained for memory used internally for type
  descriptions. This is to avoid deserializing a stream that defines a large data structure with a
  large number of members to exhaust available memory.

It is also possible to set all of these variables at the same time by setting `maxSize`. After
creating an `ObjIStream`, all of them are set to the maximum `Nat` value, which corresponds to just
below 4 GiB.


Making classes serializable
----------------------------

Usually, you don't want to serialize a large amount of primitive types. Usually, it makes more sense
to create a type that represents the data you want to serialize, and treat it as a whole unit. Of
course this is possible in Storm as well, and as long as all data members are serializable, Storm
can generate the serialization code for you automatically.

The function `util.serialize.serializalble(Type)` does just that. Call `serializable` and give a
type, and the function will examine the type and generate `write` and `read` functions that
correspond to the data in the class. The function is designed to be usable with
[decorators](md://Basic_Storm/Types) in Basic Storm. As such, in Basic Storm (and other languages
with similar mechanisms), one can easily make a class serializable as follows:

```
class MyClass : serializable {
    Int a;
    Str b;
    OtherClass c;
}
```


The serialization interface
----------------------------

There might be times when you need to serialize types that consist of non-serializable types, or
when the standard representation is not suitable for some reason. This is also possible to solve,
but requires more knowledge on the interfaces used by the serialization.

The serialization interface assumes that all serializable types (except for primitive types) contain
four functions (two of which are `read` and `write` discussed earlier) as follows:
- `void write(T this, ObjOStream to)`: Write the object to the stream, as mentioned earlier.
- `T read(ObjIStream from)` (static): Read an object from the stream, as mentioned earlier.
- `SerializedType serializedType()` (static): Generate a type description of this type for the serialization. This
  function is called when it is needed by the serialization system. You typically don't need to call it directly.
- `void __init(T this, ObjIStream from)` (constructor): Initialize the object from a stream, used while reading.

All of these functions are generated automatically when using the `serializable` function mentioned
earlier. However, all types implementing `write`, `read` and `serializedType` are considered
serializable, and as such it is possible to make all types serializable by implementing these
functions manually. Therefore, we will take a closer look at each function below:

### The write function

The write function is responsible for writing the entire object to the stream. This function is
responsible for informing the `ObjOStream` of the extents of the object by calling the `startX` and
`end` functions as appropriate.

The write function is expected to start by calling `startValue` if the type is a value or
`startClass` if the type is a class type. `startValue` takes one parameter, a `Type` value that
corresponds to the type to be serialized (it is expected to have a `serializedType` member as
above). `startValue` then returns a boolean indicating whether or not this instance needs
serialization. If serialization is required, the read function is expected to serialize any data
contained in the object, and then call the `end` function of the `ObjOStream` class to indicate that
serialization is complete. If the class has a superclass, the superclass is expected to be
serialized before any data is serialized by calling the superclass' `write` function. Serialization
of the contained data depends on the contents of the `SerializedType` provided, as will be explained
later.

If the serialized object is a class type, the `write` function is expected to call `startClass`
instead of `startValue`. `startClass` works very much like `startValue`, but in order to properly
handle shared instances and cyclic hierarchies, `startClass` takes an additional parameter: a
reference to the instance being serialized. Except for this difference, serialization of class types
work the same as for value types. As such, a `write` function usually looks like this:

```
class MyClass {
    Int a;
    Str b;

    // other members...

    void write(ObjOStream to) {
        if (to.startClass(named{MyClass}, this)) {
            // call super:write(to) if it exists
            a.write(to);
            b.write(to);
            to.end();
        }
    }
}
```

Note that `end` is not called if `startClass` or `startValue` returns `false`.

The write function is the only function (except for `serializedType`, if used) that is required for
serialization of objects.

### The read function

Since the built-in serialization attempts to be robust, deserialization is a bit more complicated
than serialization. Deserialization starts in the `read` function. However, the `read` function is
usually very simple, and can be implemented automatically by the `serializable` function provided
the other three functions are present. The `write` functions job is simply to call `readValue` or
`readClass` depending on what is being read from the stream.

The `readValue` function takes two parameters: a `Type` instance corresponding to the type being
deserialized and a raw pointer to previously allocated memory big enough to hold an instance of the
deserialized type. Because of this generic, low-level API, it is not generally possible to call this
function from higher level languages, such as Basic Storm, since they do not allow access to raw
memory in this fashion. However, if the other three functions are present, the `serializable`
function will happily generate a suitable `read` function for any type.

The `readClass` function is a bit better in this regard, as it only takes a `Type` parameter
indicating the type to deserialize. The resulting instance is returned in the form of an `Object`
reference. As such, the `read` function for class types can be implemented as follows (note that the
implementation provided by the `serializable` function is slightly more efficient, as it skips the
type check required in Basic Storm):

```
class MyClass {
    // ...
    MyClass read(ObjIStream from) : static {
        if (x = from.readClass(named{MyClass})) {
            x;
        } else {
           // Or something else...
           MyClass();
        }
    }
}
```

Both `readClass` and `readValue` will locate the `serializedType` member of the type that is to be
deserialized (in the case of `readClass`, it may be a subclass), call that function, and call the
deserialization constructor contained in the resulting `SerializedType` class to create an instance
of the deserialized type. Do not assume that objects are created in the same order that the
corresponding 'read' functions are called; the `ObjIStream` may need to cache objects when the order
of members in the stream don't match the order of members in the current code, and it may even
re-use previously created instances in case multiple references refer to the same object.


### The serializedType function

The `serializedType` function is sometimes called by the serialization mechanism during
deserialization to retrieve type information about arbitrary types in the system. The task of this
function is therefore simple: return an instance of `SerializedType` that describes the aspects of
the type relevant for serialization.

There are three subclasses of `SerializedType` that are recognized by the serialization system:
`SeralizedStdType`, `SerializedTuples` and `SerializedMaybe`. All of these subclasses, or the base
class, may be returned from the `serializedType` function. Each subclass represent a standard
serialization format that can be interpreted without having access to the implementation of the
class itself. The base class, however, represents an object that is serialized in a nonstandard way
and thus not possible to deserialize without access to the actual implementation.

- `SerializedType`: Contains the minimum amount of information required by the serialization system
  to function: A reference to the `Type` and a pointer to the read constructor used for
  deserialization. If a `serializedType` returns an instance of this type, it indicates that the
  serialized representation of the type is custom. Since the high-level serialization relies on type
  information that is not available during custom serialization, custom serialization functions may
  **not** call any serialization functions for other objects. Custom serialization **must** instead
  call low-level operations on the underlying stream (available as a member variable of the
  `ObjOStream` and `ObjIStream`). It is still possible to serialize primitive types in this manner,
  as they provide `read` and `write` functions for regular `OStream` and `IStream` objects.
- `SerializedStdType`: Indicates that the type is a standard type, consisting of a number of named
  member variables. Thus, this class contains the name of each member variable and its type. The
  type is expected to serialize its members in the order specified in this type by calling the
  corresponding `write` and `read` functions. This is the type generated by `serializable` if
  nothing else is specified.
- `SerializedTuples`: Indicates that the type consists of a number of tuples, each tuple consisting
  of a fixed number of elements with pre-defined types. Thus, this class contains information on
  which types are stored in each tuple. The type is expected to serialize its data by first writing
  a `Nat` containing the number of tuples stored, followed by that many tuples stored by calling the
  `write` or `read` function of each type in the tuple the specified number of times. This type is
  used by the standard containers, such as `Array` and `Map`.
- `SerializedMaybe`: Indicates that the type may contain a value of some type. The class contains
  information about a single type that may be stored in the stream. The type is expected to be
  serialized by writing a single `Bool` that indicates if the type is present or not. If so, the
  bool is followed by the type itself. This type is used by the `Maybe` type.

In most languages, it is necessary to implement this function by creating the desired instance each
time the function is called, which is slightly inefficient (the object streams make sure to only
call the function once for each instance). The `serialized` function generates a version that always
returns the same instance, which makes this implementation slightly more efficient. If a `write`
function and a read constructor is present, the `serialized` function will generate a
`serializedType` function that returns an instance of `SerializedType` to indicate that the type has
custom serialization.

### The read constructor

The read constructor takes a single `ObjIStream` as a parameter, and is responsible for populating
an object from a stream, the opposite of the `write` function. As previously mentioned, the read
constructor is called as needed by the serialization system, and it is located using the
`serializedType` function.

The read constructor is usually a bit simpler than the `write` function. Since the `read` function
is responsible for calling `readX` (corresponding to `startX` for `write`), the read constructor
does not need to do that. The read constructor can simply assume that an instance of the current
type is to be read from the stream immediately. For both value and class types, the constructor is
assumed to read the data for the class in the same way data is written (except, of course, calling
`read` instead of `write`), and using the retrieved data to initialize the object. If the object has
a superclass, the superclass is expected to be read before any members of the subclass. As such,
read constructors are typically implemented as follows in Basic Storm:

```
class MyClass {
    Int a;
    Str b;

    // other members...

    init(ObjIStream from) {
        // call super(from) if it exists
        init {
            a = Int:read(from);
            b = Int:read(from);
        }
        from.end();
    }
}
```

If you implement custom serialization and deserialize an array, you are additionally expected to
call the `checkArrayAlloc` function before allocating the array. This allows the `ObjIStream` to
keep track of size limits properly. If the limits are exceeded, the `checkArrayAlloc` function
throws an appropriate error.

### Examples

To summarize custom serialization, we provide two examples in Basic Storm. The first example shows
how one can implement the standard serialization using the custom serialization mechanisms described
above. Here, we have class with two members, `a` and `b` that are serialized:

```
class MyClass : serializable /* provides "read", which is difficult to implement in Basic Storm */ {
    Int a;
    Str b;

    SerializedType serializedType() : static {
        SerializedStdType t(named{MyClass}, &__init(MyClass, ObjIStream));
        t.add("a", named{Int});
        t.add("b", named{Str});
        t;
    }

    void write(ObjOStream to) {
        if (to.startClass(serializedType(), this)) {
            a.write(to);
            b.write(to);
            to.end();
        }
    }

    init(ObjIStream from) {
        init {
            a = Int:read(from);
            b = Str:read(from);
        }
        from.end();
    }
}
```

The next example shows how to implement custom serialization of the same class. This version will
produce a serialized stream that does not contain enough information to read without the
implementation of `MyClass`. Note, that this is unnecessary, as `MyClass` is simple enough to be
serializable using the standard mechanisms:

```
class MyClass : serializable /* provides "read" and "serializedType" */ {
    Int a;
    Str b;

    void write(ObjOStream to) {
        // Note: 'serializedType' is generated by the 'serializable' decorator.
        if (to.startClass(serializdType(), this)) {
            // Note: We're writing to the underlying 'OStream' rather than the 'ObjOStream'.
            OStream stream = to.to;
            a.write(stream);
            b.write(stream);
            to.end();
        }
    }

    init(ObjIStream from) {
        // Note: We're reading from the underlying 'IStream' rather than the 'ObjIStream'.
        IStream stream = from.from;
        init {
            a = Int:read(stream);
            b = Str:read(stream);
        }
        from.end();
    }
}
```

Robustness of the format
-------------------------

When storing data for longer periods of time, it is important to know what changes can be made to
the serialized classes without losing the ability to read previously stored data. For this reason,
this section describes what changes are handled by the serialization mechanism, and which are not.

All of the following scenarios assume that some data structure is serialized to an external
location. The data representation is then changed as indicated before the previously stored data is
deserialized again. Furthermore, we assume that the implementation of any classes that have custom
serialization remain unchanged, as the serialization system can make no guarantees about them. The
serialized data may, however, include types that use custom serialization as long as that
implementation remain compatible with the previous version.

### Member variables

Member variables are usually stored in the serialized stream in the order they were declared in the
class (or, more precisely, the order in which they are stored in memory, which currently correspond
to the order the member variables are declared, but may change in the future). Thus, it is
reasonable to question if re-ordering the member variables break compatibility.

Indeed, changing the order of member variables will change the order in which they are written to
the stream. However, the serialization system will detect this using the names of the members and
act as if the members appear in the order the deserializing program expects them to. This does,
however, cause a small impact to deserialization performance as it requires additional
bookkeeping. This does, however, mean that care must be taken when renaming member variables in
order to not break compatibility. Compatibility can be retained by implementing custom serialization
as shown above, and using the old name for the variable in the `SerializedStdType` instance returned
from `serializedType`. In the future, it will be possible to specify this without implementing
serialization manually.

Adding and removing member variables is not supported. In theory, nothing prevents deserializing a
stream that contains a member variable that has been removed in the current version, as long as all
types that use custom serialization remain in the system. However, this is not currently
allowed. Similarly, adding new member variables is also possible as long as all these variables
specify a value to use when they are not present in the stream. Default initialization for members
would be perfect for this situation, but since Storm does not yet support default initializers, this
is not yet supported by the serialization either. Instead, one can add new member variables by
adding a new subclass that contains the new variables. This would, however, make older versions of
the software unable to read streams from newer versions.

### Types

It is possible to deserialize a stream into an object representation as long as all types used in
the stream are still present in the program. All types need to have the same name and reside in the
same package as the time they were serialized. If inheritance is used, the superclasses of all
serialized objects also need to remain the same. It is, however, possible to deserialize a value
type into a class type (assuming the contents match), but not vice versa.

The serialization system only examines the types that actually appear in a stream. This means that
types that *could* appear in a stream, but do not actually appear in the stream, may be changed in
any way without impacting the ability to deserialize streams. This means that it is possible to add
new subclasses to previously serialized classes without impairing the ability to deserialize old
streams, and newly serialized streams will be readable by old implementations as long as the new
classes are not actually serialized.


The binary format
------------------

Primitive types are encoded as follows:

- `Bool`: A single byte containing the value 0 for `false`. Other values (usually 1) indicate `true`.
- `Byte`: A single byte containing the value.
- `Int` and `Nat`: 4 bytes stored in big endian.
- `Long` and `Word`: 8 bytes stored in big endian.
- `Float`: The IEEE754 representation stored as a `Nat`.
- `Double`: The IEEE754 representation stored as a `Word`.
- `Str`: A `Nat` containing the length of the string followed by that many bytes of data, encoded in UTF-8.

Each serialized type is assigned a type id in the form of a `Nat` in order to refer to types in an
efficient manner. Type identifiers may appear before they are defined in the stream, for example
when referring to member types in a class. However, types are always defined whenever the first
instance of that particular type is read or written to the stream, so that the reader knows the
format of the serialized objects without access to the actual implementation. Primitive types
have pre-defined ids, defined by the enum `core.io.StoredId`. The id `core.io.StoredId.endId` (value
0) is of special interest. It is used to indicate the absence of a type, or the end of a
list. Identifiers below `core.io.StoredId.firstCustomId` are reserved for future primitive types,
and shall not be used.

A type description is stored as follows:
- A `Byte` indicating what kind of type is stored, described by the `core.io.TypeInfo` enum.
- A `Str` containing the name of the type, serialized as follows: The format is similar to the
  textual representation of `core.lang.Name`. Each part is stored as a string ending with the ASCII
  character 1. If parameters are present, they are surrounded by the ASCII character 2 and 3 (for
  start- and end parentheses). Each parameter ends in either the ASCII character 4 if passed by
  value or 5 if passed by reference.
- A `Nat` containing the id of the parent type, or `core.io.StoredId.endId` if none.

The remainder of the type description depends on which values are present in the first byte
(`core.io.TypeInfo` is a bitmask):
- `tuple`: This is an array of tuples. A series of `Nat` values containing the id of each type
   in the tuple are stored. The end of the list is indicated by `core.io.StoredId.endId`.
- `maybe`: This is a maybe-type. A single `Nat` containing the contained type id is stored.
- `custom`: This is a type with custom serialization. Noting more is stored, but the serialized
  data for instances of this type can not be extracted without asking the actual type.
- Otherwise: This is a regular class- or value type (depending on the presence of `classType`).
  The members of the class or value are stored as pairs, first a `Nat` containing the type id of
  the member, followed by a `Str` containing the member's name. The end of the list is indicated
  by a type with id `core.io.StoredId.endId`. No string follows the last type id.

Each object in a session starts with a single `Nat` describing the type of the stored object,
followed by the object itself (which, for the first object in a session, starts with a type
definition unless a primitive type was stored). Types are then stored depending on their type
(described by the type description) as follows:

If the stored value is a class type (i.e. `core.io.TypeInfo.classType` is not present in the first
byte of the type description), a `Nat` describing the object's id is stored first. If this id was
serialized before, nothing more is stored; this is a reference to the previously stored
object. Otherwise, a `Nat` containing the type of the object follows (as the dynamic type might
differ from the static type). If this is a new type id, the type description directly follows the
new id, as we need to access that. If any parent classes are not yet serialized, they also
follow. Finally, the data of the object is stored as described below.

Values are simpler, as their static type always matches their dynamic type, and they are always
copied. Therefore, values are always stored as the data of the object, without the additional
headers required for storing class types.

The data of the object also depends on the first byte of the type description as follows:
- If the type is a built-in type, use the default serialization of that type (the `read` and `write`
  functions for a regular `IStream` and `OStream`).
- If the type is an array of tuples, a `Nat` containing the number of elements in the array, followed
  by that many repetitions of the types in the type description.
- If the type is a maybe type, a `Bool` indicating the presence of a value is stored first. If the
  value is true, then a single instance of the contained type is stored directly afterwards.
- If the type is a custom type, the representation is unspecified.
- If the type is a regular class- or value type, each member is serialized in the order specified
  by the type description.


Example
---------

Consider the code below, found in the file `serialization.bs` in the `demo` package:


```
?Include:root/demo/serialization.bs?
```

The function `serializeExample` serialize an instance of the `Wrap` class and as the first session,
followed by an array of `Val` objects as the second session. Then, the textual representation is
printed followed by the binary representation. The textual representations illustrate how the data
is structured by the serialization mechanism, and should be fairly apparent from the output:

Session 1 (the `Wrap` object):
```
demo.Wrap (instance 0) {
    a: demo.Val {
        a: 1i
        b: "One"
    }
    b: demo.Val {
        a: 2i
        b: "Two"
    }
    c: demo.Derived (instance 1) {
        a: 3i
        b: 4i
    }
    d: demo.Base (instance 2) {
        a: 5i
    }
    e: <link to instance 2>
}
```

Session 2 (the array of `Val` objects):
```
core.Array(demo.Val) (instance 0) [
    demo.Val {
        a: 10i
        b: "Ten"
    }
    demo.Val {
        a: 20i
        b: "Twenty"
    }
]
```

The data is represented in the stream as follows. The data is annotated to show how each part is
represented by the format.

```
00 00 00 20    // Type-id of the root object of this session (32, user defined)
               // This is a type we don't know about, so we read a type description:
01             // -Type flags (class type)
               // -Next is a string containing the mangled name of the class:
00 00 00 0A    // --Length of the string (10 bytes)
64 65 6D 6F    // --"demo" in UTF-8
01             // --End of this part.
57 72 61 70    // --"Wrap" in UTF-8
01             // --End of this part.
00 00 00 00    // -Parent type (none)
               // -This is a compound type, so we read its members.
00 00 00 21    // -Type-id of the first member (33, user defined)
00 00 00 01 61 // -Name of the member (string, "a")
00 00 00 21    // -Type-id of the second member (33, user defined)
00 00 00 01 62 // -Name of the member (string, "b")
00 00 00 22    // -Type-id of the third member (34, user defined)
00 00 00 01 63 // -Name of the member (string, "c")
00 00 00 22    // -Type-id of the fourth member (34, user defined)
00 00 00 01 64 // -Name of the member (string, "d")
00 00 00 22    // -Type-id of the fifth member (43, user defined)
00 00 00 01 65 // -Name of the member (string, "e")
00 00 00 00    // -No more members (type-id 0 is not used)
00 00 00 00    // Instance of this class (0, not present earlier)
               // Since instance 0 was not serialized earlier, we read the data now
00 00 00 20    // Actual type of the instance (32, Wrap)
               // Start reading an instance of type 33 for "a".
               // -Type 33 is not known yet, read the type description first.
00             // --Type flags (value type)
00 00 00 09    // ---Length of the type name (9 bytes)
64 65 6D 6F 01 // ---The first part ("demo")
56 61 6C 01    // ---The second part ("Val")
00 00 00 00    // --Parent type (none)
00 00 00 03    // --Type-id of the first member (3, Int)
00 00 00 01 61 // --Name of the member (string, "a")
00 00 00 09    // --Type-id of the second member (9, Str)
00 00 00 01 62 // --Name of the member (string, "b")
00 00 00 00    // --No more members
               // -Now, we start reading the "Val" instance for "a" inside "Wrap"
00 00 00 01    // -Value for "a" inside the "Val" instance (1)
00 00 00 03    // -Length of the string for "b" in the "Val" instance (3 bytes)
4F 6E 65       // --Content ("One")
               // Done reading "Val", continue reading an instance for "b".
00 00 00 02    // -Value for "a" inside the "Val" instance (2)
00 00 00 03    // -Length of the string for "b" in the "Val" instance (3 bytes)
54 77 6F       // --Content ("Two")
               // Trying to read an instance of id 34 for "c".
	       // -Type 34 is not known yet, read the type description first.
01             // --Type flags (class type)
00 00 00 0A    // --Length of the type name (10 bytes)
64 65 6D 6F 01 // ---The first part ("demo")
42 61 73 65 01 // ---The second part ("Base")
00 00 00 00    // --Parent type (none)
00 00 00 03    // --Type-id of the first member (3, Int)
00 00 00 01 61 // --Name of the member (string, "a")
00 00 00 00    // --No more members
00 00 00 01    // -Instance id (1, not present earlier)
00 00 00 23    // -Actual type (35) differs from formal type, previousl unknown.
               // -Trying to read an instance of 35, we need a type description.
01             // --Type flags (class type)
00 00 00 0D    // --Length of the type name (13 bytes)
64 65 6D 6F 01 // ---The first part ("demo")
44 65 72 69    //
76 65 64 01    // ---The second part ("Derived")
00 00 00 22    // --Parent type (34, Base)
00 00 00 03    // --Type of the first member (3, Int)
00 00 00 01 62 // --Name of the member (string, "b")
00 00 00 00    // --No more members
00 00 00 03    // -Data for "a" inside "Base" (3)
00 00 00 04    // -Data for "b" inside "Derived" (4)
               // Continue reading an instance of type 34 for "d"
00 00 00 02    // -Instance id (2, not present earlier)
00 00 00 22    // -Actual type (34, Base)
00 00 00 05    // -Value for "a" inside "Base")
               // Continue reading an instance of type 34 for "e"
00 00 00 02    // -Instance id (2, previously known, reuse that instance)
               // Done reading "Wrap". Session 1 completed.

00 00 00 24    // Start of session 2, type-id of the first type (36, not known)
03             // -Type flags (class type, tuple)
00 00 00 17    // -Length of the type name (23 bytes)
63 6F 72 65 01 // --The first part ("core")
41 72 72 61 79 // --The second part: "Array"
02 64 65 6D 6F // --"<demo"
01 56 61 6C 01 // --".Val."
04 03 01       // --",>."
00 00 00 00    // -Parent type (none)
00 00 00 21    // -First type in each tuple (33, Val)
00 00 00 00    // -End of the list
00 00 00 00    // Instance id (0, not present earlier in this session)
00 00 00 24    // Actual type (36, Array<demo.Val>)
00 00 00 02    // Number of elements (2)
               // Reading element 0, type 33
00 00 00 0A    // -Value for "a" (10)
00 00 00 03    //
54 65 6E       // -Value for "b" (string, "Ten")
               // Reading element 1, type 33
00 00 00 14    // -Value for "a" (20)
00 00 00 06 54 //
77 65 6E 74 79 // -Value for "b" (string, "Twenty")
               // Done reading "Array<demo.Val>". Session 2 completed.
```