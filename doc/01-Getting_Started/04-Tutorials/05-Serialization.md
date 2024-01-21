Serialization in Storm
======================

This tutorial illustrates how the [serialization
library](md:/Library_Reference/Standard_Library/IO/Serialization) works by using two examples. The
first example uses the serialization library to save settings for a program. The second example uses
the serialization library to implement a simple but robust communication protocol.

The code presented in this tutorial is available in the directory `root/tutorials/serialization` in
the Storm release. There are a few different entry points that can be run from the Basic Storm
interactive top-loop:

- `tutorials:serialization:save` - Save the settings in the first part of the tutorial.
- `tutorials:serialization:text` - Load settings as text, to inspect what is stored in the saved file.
- `tutorials:serialization:load` - Load settings in the first part of the tutorial.
- `tutorials:serialization:protocol` - Run the message protocol from the second part of this tutorial.


Saving Settings
---------------

In the first part of the tutorial we will use the serialization library to save settings for a
fictional program to disk and load them back into memory. We will also explore the abilities of the
serialization library to load data saved from earlier versions of a program.

### Setup

For this part of the tutorial we will work in a single file named `settings.bs`. As a start, add the
following contents to the file:

```bs
void save() {
    print("TODO");
}
```

Then open a terminal and change to the directory where you saved the file. You should be able to run
the code by typing:

```
storm settings.bs -f settings.save
```

If done successfully, Storm should print `TODO` and exit.


### Data

The next step is to define some data that we can serialize. For the purposes of this tutorial, we
create a class `Settings` that we imagine contains all the settings that our imaginary program
wishes to save. As a part of the settings, we also create a class `User` that stores basic
information about the user. We implement the classes as follows:

```bs
class Settings {
    User user = User("Filip", 30);
    Str country = "Sweden";

    void toS(StrBuf to) : override {
        to << "{ user: " << user << ", country: " << country << " }";
    }
}

class User {
    Str name;
    Nat age;

    init(Str name, Nat age) {
        init { name = name; age = age; }
    }

    void toS(StrBuf to) : override {
        to << "{ name: " << name << ", age: " << age << " }";
    }
}
```

In the `Settings` class, we use default values for the data members to make it easier to create
instances of the `Settings` object with interesting contents. This may or may not be suitable in
real applications.


### Serialization

To save the contents of the two classes, we first need to make them serializable. Typically, this is
done by adding the `serializable` decorator to the affected classes. The logic for this decorator
resides in the package `util:serialize`, so that package needs to be included as well:

```bs
use util:serialize;

class Settings : serializable {
    // ...
}

class User : serializable {
    // ...
}
```

After that, we can turn our attention to the actual serialization logic. To serialize objects, we
need to use the `ObjOStream` class (for *object output stream*). As with the text streams in the
previous tutorial, the `ObjOStream` accepts an `OStream` as a parameter to its constructor that
specifies where data should be stored.

After creating an `ObjOStream`, we can write objects to them by calling the member function `write`
in the object we wish to store. This function is generated automatically by the `serializable`
decorator we added earlier.

All in all, we can implement the serialization in the `save` function as follows:

```bs
use core:io; // Add to the top of the file

Url settingsUrl() {
    return cwdUrl / "settings.dat";
}

void save() {
    Settings settings; // Object we wish to save.

    Url saveTo = settingsUrl();
    ObjOStream out(saveTo.write()); // Create an ObjOStream
    settings.write(out);            // Write the object
    out.close();                    // Close the stream.

    print("Saved to: ${saveTo}");
}
```

We can run the program by typing `storm settings.bs -f settings.save` in the terminal. This
serializes the settings object into the file `settings.dat` in the current working directory (the
program outputs the path as well).


### Inspecting the Serialized Data

If you open the `settings.dat` file you serialized previously, you will see that it uses a binary
format. We do, however, see some strings in the file that contains names of the types that were
stored. In fact, the serialized data contains enough information to deserialize the serialized data
without access to the original class definitions. It is possible to use this data to convert the
binary stream into a textual, human-readable format using the class `TextObjStream` that is
implemented in the `util.serialize` package.

We can use the stream to read our serialized data as follows:

```bs
void text() {
    Url loadFrom = settingsUrl();
    TextObjStream in(loadFrom.read());

    Str textRepr = in.read();
    print("Settings as text:");
    print(textRepr);
}
```

If we run the program by typing `storm settings.bs -f settings.text` in the terminal, we will get
the following output (note that the class names are different if you use the code in the Storm
release):

```
settings.Settings (instance 0) {
    user: settings.User (instance 1) {
        name: "Filip"
        age: 30n
    }
    country: "Sweden"
}
```

Objects are represented by a class name followed by a pair of curly braces that denotes the contents
of the object. For class types, we also see `(instance N)` that indicates which instance of the
class is being defined. This is sometimes used by the textual representation to refer back to
previously stored objects, in case the object graph contains multiple references to the same object.


### Deserialization

Now that we know that our data was serialized properly, we can deserialize the data into a
`Settings` object again. We do this using the `ObjIStream` object together with the static `read`
function that was generated by the `serializable` decorator. Similarly to when serializing, we first
open the file as a plain `IStream`, which we then pass to the constructor of the `ObjIStream`:

```bs
void load() {
    Url loadFrom = settingsUrl();
    ObjIStream in(loadFrom.read());

    print("Loading settings from: ${loadFrom}...");

    try {
        Settings settings = Settings:read(in);
        print("Success: ${settings}");
    } catch (SerializationError e) {
        print("Failed: ${e.message}");
    }
}
```

If we run the program by typing `storm settings.bs -f settings.load` in the terminal, we will get
the following output:

```
Loading settings from: /home/storm/settings.dat...
Success: { user: { name: Filip, age: 30 }, country: Sweden }
```

### Changing the Settings Class

The serialization library is able to support changes to classes to a certain degree. This means that
it is possible to load serialized data from an old version of the system, even if changes were made
to the serialized classes.

To illustrate this, let's assume that we have developed our imaginary program further and we wish to
add a new setting for the user's preferred language. We can add the setting by modifying the
`Settings` class as follows:

```bs
class Settings : serializable {
    User user = User("Filip", 30);
    Str country = "Sweden";
    Str language;

    void toS(StrBuf to) : override {
        to << "{ user: " << user << ", country: " << country << ", language: " << language << " }";
    }
}
```

If we run the `load` function at this time, deserialization will fail with the following message:

```
Failed: The member language, required for type Settings, is not present in the
stream and has no default value in the source code.
```

As indicated by the message, the serialization library has realized that a new member was added to
the `Settings` class compared to the time when the data file was serialized. The message also gives
a hint that it is possible to solve the issue by specifying a default value. Let's try to specify a
default value for `language` as follows:

```bsclass
Str language = "Swedish";
```

If we run the `load` function again at this point, the serialization library uses the default value
to fill in the missing value from the serialized representation, and we get the following output:

```
Loading settings from: /home/storm/settings.dat...
Success: { user: { name: Filip, age: 30 }, country: Sweden, language: Swedish }
```

It is possible to remove members from classes as well. It is, however, worthwile to be a bit careful
with renaming data members. The serialization library uses names of member variables to match data
members, and renaming a data member is thus treated as the removal of the old name and the addition
of the new name. This means that the user will lose the value of the renamed setting.

### Complex Data

The serialization library takes care to preserve the structure of the object graph during
serialization. In particular, if multiple variables in the serialized representation refer to the
same instance of an object, the object will not be duplicated, and the deserialized representation
will preserve this property.

To illustrate this, let's add an alternate user identity to the `Settings` class and make the two
refer to the same `User` instance. We also modify the `toS` function to indicate if `user` and
`alternate` refer to the same object or not.

```bs
class Settings : serializable {
    User user;
    User alternate;
    Str country = "Sweden";

    init() {
        User tmp("Filip", 30);
        init {
            user = tmp;
            alternate = tmp;
        }
    }

    void toS(StrBuf to) : override {
        to << "{ user: " << user << ", alternate: ";
        if (user is alternate) {
            to << "(same as user)";
        } else {
            to << alternate;
        }
        to << ", country: " << country << " }";
    }
}
```

We can now serialize the new object using `storm settings.bs -f settings.save`, and then inspect the
textual representation of the serialized data using `storm settings.bs -f settings.text`. The
textual representation shows the fact that `user` and `alternate` refer to the same object as
follows:

```
settings.Settings (instance 0) {
    user: settings.User (instance 1) {
        name: "Filip"
        age: 30n
    }
    alternate: <link to instance 1>
    country: "Sweden"
}
```

As we can see, the textual representation shows the value of `alternate` as `<link to instance 1>`.
If we look at the value for `user`, we can see that it has been labeled as `(instance 1)`. This
shows that the fact that they are the same instance has been preserved in the serialized
representation as well.

Of course we can deserialize the data again to verify that the deserialization logic behaves in the
same way. If we run `storm settings.bs -f settings.load`, we will see the following output that
verifies that it is indeed the case since it prints `(same as user)` as the value for `alternate`:

```
Success: { user: { name: Filip, age: 30 }, alternate: (same as user), country: Sweden }
```


Communication Protocol
----------------------

As mentioned in the top of this page, it is also possible to use the serialization library to
conveniently implement a network protocol. In this part of the tutorial, we will use the protocol
within a single process, but the same idea can be extended to communication between different
machines.


### Setup

For this part of the tutorial we will work in a single file named `protocol.bs`. As a start, we add
the function that we will use as the entry-point to the file:

```bs
use core:io;
use util:serialize;

void main() {
    print("TODO");
}
```

Then open a terminal and change to the directory where you saved the file. You should be able to run
it by typing `storm protocol.bs`. If done successfully, it should print `TODO` and then exit.


### Requests and Responses

This protocol will be a simple request-response protocol. The client sends a request to the server,
and assumes that each request will eventually produce a response of some type. It is, of course,
possible to extend the idea presented here to a more complex scheme as well. It does, however,
introduce more complexity in the implementation.

We model requests using a class that we call `Request`. To make it easy for the server to determine
how to respond to each request, we define an abstract function `execute` in the `Request` class that
implements the behavior that the server should execute. We let the `execute` function receive a
reference to a `Server` object as well, so that it may access the state that is present in the
server:

```bs
class Request : serializable {
    Response execute(Server server) : abstract;
}
```

Since responses belong to messages, we do not need the same dispatch logic there. As such, we
represent responses as an empty class that we can then inherit from to add additional data in the
responses:

```bs
class Response : serializable {
}
```

### The Server

Now that we have an idea of how we will model requests and responses, we can start implementing the
server. We will represent the server as a class that contains the state managed by the server. In
our case, we implement a server that simply stores a list of strings. It implements two requests,
one for adding an element to the list, and one for retrieving the current contents of the list.

With this in mind, we can start implementing the server class as follows:

```bs
class Server {
    Str[] data;
}
```

Now that we know how the data is stored inside the server, we can implement the request that adds a
string to the list as follows. Note that we need to store any data that should be sent from the
client to the server as members inside the class:

```bs
class AddRequest : extends Request, serializable {
    Str toAdd;

    init(Str toAdd) {
        init { toAdd = toAdd; }
    }

    Response execute(Server server) : override {
        server.data.push(toAdd);
        return Response();
    }
}
```

As we can see, we can implement the logic for how to handle the request in the `execute` function.
This will make it possible to implement the logic in the `Server` class by simply calling `execute`
on the received `Request`. This also makes it easy to extend the server with new messages in the
future, without having to modify the `Server` class itself.

In a similar way, we can implement the request for retrieving the list as follows. Since this
request needs to return some data in its response, we also need to define a subclass to `Response`
that stores the data we wish to return:

```bs
class GetRequest : extends Request, serializable {
    Response execute(Server server) {
        return GetResponse(server.data);
    }
}

class GetResponse : extends Response, serializable {
    Str[] data;

    init(Str[] data) {
        init { data = data; }
    }
}
```

Since the actual logic for handling each request is implemented in the corresponding `Request`
class, all that remains is the logic for deserializing requests and serializing the responses. We
implement this as a loop in the `Server` class as follows:

```bs
class Server {
    Str[] data;

    void run(IStream read, OStream write) {
        ObjIStream input(read);
        ObjOStream output(BufferedOStream(write));

        try {
            do {
                Request request = Request:read(input);
                Response response = request.execute(this);
                response.write(output);
                output.flush();
            }
        } catch (EndOfStream e) {
            // End of stream reached, this is normal.
        } catch (SerializationError e) {
            print("Serialization error: ${e.message}");
        }

        input.close();
        output.close();
    }
}
```

As we can see, the central parts of the server logic is inside the `do` loop inside the `try` block.
This loop simply reads a message by calling `Request:read(input)`, executes the logic associated
with the message by calling `request.execute(this)`, and finally sending the response back to the
client using `response.write(output)`.

When the client closes their end of the `input` stream, deserialization will fail with the
`EndOfStream` exception. As such, the code above catches the exception and allows execution to
continue normally in this case. Note that `EndOfStream` is a subclass of `SerializationError`, that
is used to report other forms of serialization errors. This means that the order of the `catch`
clauses are important in the code above.

A detail worth mentioning in the code above is the use of a `BufferedOStream` when creating the
`ObjOStream`. The `BufferedOStream` acts as a layer between the `ObjOStream` and the `OStream` that
collects the small writes that are performed by the `ObjOStream` and only writes them when the
internal buffer is full or when `flush` is called. This is technically not necessary in this
example, since we will send data between two threads in the same system. However, this buffering is
often very beneficial when sending data over the network, especially if encryption is used, since
there is much more overhead involved in network transmissions.

### The Client

Now that we have a server, we are ready to implement the client. We implement the client as a class
that contains two object streams, one for input and one for output. The class then contains a
function called `request` that sends a request to the server and waits for a response. We can
achieve this as follows:

```bs
class Client {
    private ObjIStream input;
    private ObjOStream output;

    init(IStream read, OStream write) {
        init {
            input(read);
            output(BufferedOStream(write));
        }
    }

    Response request(Request request) {
        request.write(output);
        output.flush();
        return Response:read(input);
    }

    void close() {
        input.close();
        output.close();
    }
}
```

As with the server, we use a `BufferedOStream` to buffer writes to the output stream. We can also
see that the `request` function is fairly simple. It starts by writing the `request` object to the
`output` stream, flushes the stream (to inform the `BufferedOStream` that it needs to send any
buffered data immediately), and reads a response from the `input` stream.

We can use the `Client` class to interact with the server as follows:

```bs
void runClient(IStream read, OStream write) {
    Client client(read, write);

    client.request(AddRequest("a"));
    client.request(AddRequest("b"));

    if (response = client.request(GetRequest()) as GetResponse) {
        print("Final data: ${response.data}");
    } else {
        print("Invalid response.");
    }

    client.close();
}
```

The code above sends two `AddRequest`s to add two elements to the array in the server. After that it
asks the server for the contents of the array with a `GetRequest`. At this point we are interested
in inspecting the result from the `request` function. As such, we first cast it into a `GetResponse`
using a weak cast, and then print the `data` member of the response.


### Connecting the Server and the Client

Finally, we wish to connect the server and the client we have written. In this tutorial we simulate
the network connection using the `Pipe` class. We create two pipes, one for moving data from the
client to the server, and another for moving data from the server to the client. We then spawn two
user threads, one for the client and one for the server, and wait for them to finish:

```bs
void main() {
    Pipe a;
    Pipe b;
    var server = {
        Server server;
        spawn server.run(a.input, b.output);
    };
    var client = {
        spawn runClient(b.input, a.output);
    };

    // Wait for both to finish.
    server.result();
    client.result();
}
```

Now, we can run the program by typing `main protocol.bs` in the terminal. If everything is done
correctly, it should produce:

```
Final data: [a, b]
```

If the program appears to freeze, the reason is likely that some part of the code failed to compile,
but the exception containing the error message is caught in the future for the client, but is not
shown since the program is waiting for the server to complete. To see the error, you can ask Storm
to compile the program before launching the server or the client by adding the following line at the
start of `main`, and add `use lang:bs:macro;` to the top of the file:

```bsstmt:use=lang.bs.macro
named{}.compile();
```

### Security Considerations

Since the program above sends entire classes from the client to the server, it might appear that it
is possible to make the server execute arbitrary code by simply sending it a new class with the
desired code in the `execute` function. This is however **not** possible. The serialized data only
contains the names of classes and their data members. No code is transmitted in the serialized
representation. As such, if the client were to send a serialized version of a class that is not
present in the server, serialization will simply fail with an exception.

One consideration when working with potentially untrusted data from the network is, however, what
happens in cases where the client (either intentionally or unintentionally) sends large data
structures to the server. This can cause the server to exhaust its memory, which could cause it to
start performing poorly. To guard against this type of behaviors, it is useful to set `maxReadSize`
and `maxArraySize` in the `ObjIStream` to sensible values in order to limit the memory used by the
deserialized objects. The default values of these variables are large in order to not cause issues
when working with trusted data. When working with potentially untrusted data it is, however,
reasonable to set them to a couple of megabytes instead.
