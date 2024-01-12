New Entities in the Name Tree
=============================

This tutorial shows how to add new entities to the name tree using a language extension to Basic
Storm. This is not the only way to add entities to the name tree. It is also possible to create a
new language that produces custom entities. This allows having a separate file-type that specifies
the new entities. See the tutorial on [creating new languages](md:New_Languages) for how to produce
entities this way.

In this tutorial we will be adding a very simple new type of entity, a note that contains a title
and a body. The goal is to be able to define notes at the top-level in Basic Storm, and to be able
to access them from the name tree. It is worth noting that the fact that we are adding notes to the
name tree is not the important part of this tutorial. The focus is on the mechanisms used to get new
things into the name tree. Using something simple, like a note, makes it easier to focus on the
relevant parts.

As with the other tutorials, the code produced by this tutorial is available in
`root/tutorials/entities`. You can run it by typing `tutorials:entities:main` in Storm's top-loop.


Setup
-----

First, we need somewhere to work. Create an empty directory somewhere on your system. This tutorial
assumes you use the name `entities`, but the name does not matter too much as long as the name is
usable as a package name in Basic Storm. If you use a different name, you will have to modify the
`use` statement in the example file.

Inside the directory, create a file named `main.bs`, and add the following contents to it. This is a
test program that illustrates the goal with the tutorial:

```
use entities:notes;
use lang:bs:macro;

note MyNote {
    My Example Note

    It can contain multiple
    lines of text in the body
}

void main() {
    print(named{MyNote}.note.toS);
}
```

Also create a subdirectory called `notes`. Since we are working with syntax, it is a good idea to
define the syntax in a separate package from where the grammar is used. As such, we will implement
the syntax in the `notes` directory. Create two files inside the `notes` directory: `notes.bs` and
`notes.bnf`. This is where we will implement the semantics and syntax respectively.

Finally, open a terminal and change into the first directory you created (where you created
`main.bs`). This makes it possible to run the example by typing:

```
storm .
```

A Class for Notes
-----------------

The first step is to create a class that represents the data we wish to add to the name tree. To
keep things simple, we consider a note to consist of a title and a body, both of which are strings.
We can store them in the class `Note` that we define in the `nodes.bs` file as follows:

```bs
class Note {
    // Title of the note.
    Str title;

    // The body text of the note.
    Str body;

    // Create.
    init(Str title, Str body) {
        init() {
            title = title;
            body = body.removeIndentation();
        }
    }

    // To string:
    void toS(StrBuf to) : override {
        to << title << "\n\n";
        to << body;
    }
}
```


The Named Entity
----------------

Since the class `Note` does not inherit from `Named`, it can not be added directly to the name tree.
While it is possible to make the Note class inherit from `Named` directly, doing so makes it into an
actor that needs to run on the `Compiler` thread. In cases where this is not a problem, or even
desirable, the class `Note` can be converted to inherit from `Named`. In this tutorial we will,
however, keep the entity separate from the data in order to allow the `Note` to be inspected without
thread switches.

Since a named entity is nothing more than an actor that inherits from the `Named` actor, we can
simply define a new actor in the `notes.bs` file as follows:

```bs
use core:lang;

class NoteEntity extends Named {
    // The Note stored in the entity.
    Note note;

    // Create.
    init(SrcPos pos, Str name, Note note) {
        init(pos, name) {
            note = note;
        }
    }
}
```

As we can see, the `Named` constructor requires a source location that tells it where it was
defined, and the name of the entity. The `pos` parameter is technically optional, but without it
modifiers such as `private` will not work properly, so it is a good idea to provide it whenever
possible.


Defining the Grammar
--------------------

After we have defined our `Note` type and the corresponding entity, it is time to define the grammar
for defining notes in Basic Storm. Since we will extend the Basic Storm language, we start by
specifying that we wish to use the delimiters from Basic Storm:

```bnf
use lang.bs;

optional delimiter = SDelimiter;
required delimiter = SRequiredDelimiter;
```

After that, we can start implementing the actual grammar. Basic Storm uses the rule `SPlainFileItem`
for any declarations that may appear on the top-level in a file. Since we wish to declare our notes
at this level, we start by extending that rule:

```bnf
SPlainFileItem : "note" #keyword ~ SName name #typeName, "{", SNote note, "}";
```

Note that the required delimiter is used between the keyword `note` and the name of the note.
Without it, the grammar would match `noted` as `note d`, which does not follow the expectations of
the users of our library.

Matching a note is delegated to the `SNote` rule that can be implemented as follows:

```bnf
Note SNote();
SNote => makeNote(title, body)
  : "[^} \n\t][^{}\n]*" title - "\n" - "[ \t]*\n" - "[^{}]*" body;
```

The `SNote` rule first matches a single line with text and binds the matched text to the variable
`title`. After that it expects a newline, followed by a blank line. Finally, it matches any
remaining content before the closing bracket. To avoid confusion, we also exclude a nested opening
bracket. Otherwise, notes like the one below would confusingly enough be acceptable:

```
note ConfusingNote {
    title{

    body{
}
```

If you are interested in allowing nested pairs of brackets inside the note syntax, have a look at
the `SSkipBlock` rule in the Basic Storm grammar in `root/lang/bs/syntax.bnf` for inspiration.

In the production to `SNote` above, we also use the transform function to create the node in the
grammar by calling `makeNote`. This function is implemented in `notes.bs` as follows:

```bs
Note makeNote(Str title, Str body) {
    Note(title, body.removeIndentation());
}
```

The reason why we call `makeNote` instead of calling the constructor directly is to be able to call
`removeIndentation` on the `body` to remove leading whitespace on all lines.


Adding the Entity to the Name Tree
----------------------------------

At this point, we have a grammar that is able to parse our extension, but not much more. If we run
the program at this point, we will get an error similar to the one below:

```
@/home/storm/entities/notes/notes.bnf(195-272): Syntax error:
No return value specified for a production that does not return 'void'.
```

This means that Basic Storm was able to parse our code properly, but it failed to transform the
parse tree into an abstract syntax tree, since we have not specified a transform function for the
`SPlainFileItem` production we created.

To understand what the production is supposed to return, we need to understand Basic Storm's loading
process in a bit more detail. Basic Storm first parses the entire source file into a parse tree.
After doing that, it transforms the topmost levels of the parse tree into an abstract syntax tree.
At this stage, Basic Storm has a collection of top-level constructs that should be inserted in the
name tree at some point. However, since some entities depend on other entities from the same or
other files in the same package (e.g. function parameters), it needs to take some care with the
order in which the entities are created. For this reason it first represents the entities as
`NamedDecl` actors. As such, we need to create a `NamedDecl` subclass to follow this convention. The
implementation needs a constructor that stores the relevant information. It also needs to override
the function `doCreate` to actually create the entity (there is also a function called `doResolve`
if names need to be resolved relative other entities in the same package).

```bs
class NoteDecl extends lang:bs:NamedDecl {
    SStr name;
    Note note;

    init(SStr name, Note note) {
        init {
            name = name;
            note = note;
        }
    }

    protected Named doCreate() : override {
        NoteEntity(name.pos, name.v, note);
    }
}
```

With the class defined, we can now add the transform function to the production as follows:

```bnf
SPlainFileItem => NoteDecl(name, note)
  : "note" #keyword ~ SName name #typeName, "{", SNote note, "}";
```

At this point, we can run the program and observe that it actually works as expected. We can now use
`Named` to retrieve the `NoteEntity` entity we inserted in the name tree, and inspect it as any
other entity.


Convenient Access
-----------------

Since the `NoteEntity` entity is a completely new entity, most languages in Storm will not know how
to handle it. For example, if we change the `main` function in `main.bs` into the following:

```bs
void main() {
    print(MyNote.toS);
}
```

We will get the following error message:

```
@/home/storm/root/tutorials/entities/main.bs(253-259): Type error:
entities.MyNote refers to a entities.notes.NoteEntity. This type is not natively
supported in Basic Storm, and can therefore not be used without custom syntax.
Only functions, variables, and constructors can be used in this manner.
```

As we can see, Basic Storm found the entity, but does not know how to do anything useful with it.
One way to achieve this would be to modify Basic Storm to teach it what to do with a `NoteEntity`.
Another, easier way is to implement our `NoteEntity` in terms of other named entities that other
languages already understand. In our case, we wish to make it convenient to get access to the
contained `Note` object. As such, we can chose to implement the entity as a function that returns
the `Note`.

To do this, we update our `NoteEntry` actor to inherit from `Function` instead of `Named`. To do
this, we must also supply a return type and parameter types to the parent constructor:

```bs
use lang:bs:macro;

class NoteEntity extends Function {
    // The note stored in the entity.
    Note note;

    // Create.
    init(SrcPos pos, Str name, Note note) {
        init(pos, named{Note}, name, []) {
            note = note;
        }
    }
}
```

If we run the program now, we will observe that Storm crashes. This is because we have defined a
function, but we have not yet specified the body of the function. Internally, this means that the
function points to address 0, which causes the crash.

To supply a behavior, we need to call `setCode` with a suitable subclass to
[stormname:core.lang.Code]. In this case, [stormname:core.lang.DynamicCode] suits our needs, as it
supports converting a [stormname:core.asm.Listing] into machine code that can be executed later on.
We could also use [stormname:core.lang.LazyCode] to generate the function body lazily. This is,
however, not necessary in this case as the function we will generate is very short and simple.

Before generating code, we must briefly consider the threading model of Storm. Since the goal of the
function is to return a pointer to the `Note` that is stored inside the `NoteEntity`. Furthermore,
the `Node` type is a class, which means that it should not be possible for different OS thread to
have a reference to the same instance of it. Since we are working at a low level, we need to make
sure that we uphold this property. Since the function we are about to create is not associated to a
particular thread, simply returning the `Note` would violate this rule. We have two options to avoid
violating the rule:

1. Associate the function with some thread (e.g. the `Compiler` thread). This means that calls to
   the function that originate from different OS threads will have to send a message to the
   designated thread. This process involves copying the result returned from the function to avoid
   shared data.

2. We can simply copy the result ourselves before returning it. While this means that different
   threads technically have access to the same data concurrently, it is not an issue since all
   threads are reading from the shared data (assuming, of course, that the `note` in the
   `NoteEntity` is not used to do so). Since this avoids messaging between threads, we take this
   approach in this tutorial. We can avoid the assumption of "no modifications through the `note`
   variable" by copying the note before storing a reference to it in the generated machine code.

With this in mind, we can generate the required code as follows:

```bs
use lang:bs:macro;

class NoteEntity extends Function {
    // The note stored in the entity.
    Note note;

    // Create.
    init(SrcPos pos, Str name, Note note) {
        init(pos, named{Note}, name, []) {
            note = note;
        }

        Listing l(false, ptrDesc);
        l << prolog();
        l << fnParam(ptrDesc, objPtr(note));
        l << fnCall(named{clone<Note>}.ref, false, ptrDesc, ptrA);
        l << fnRet(ptrA);

        setCode(DynamicCode(l));
    }
}
```

The parameters to the `Listing` consttructor indicates that we are not generating a member function,
and that the function returns a pointer to something. Next, we add the function prolog which sets up
a stack frame for the function.

After the prolog, we are ready to call the `clone(Note)` function that Storm automatically generates
for us. We start by emitting the `fnParam` instruction to specify the parameter to the function.
Here, we specify that we will pass a pointer parameter to the function, and that the parameter's
value is a pointer to the object we have in `note`. This operation copies the pointer in the
variable `note`, rather than creating a reference to the variable. This means that if we wish to
have an extra layer of protection against accidental modifications of shared data, we could simply
make a copy of the contents of the `note` variable before storing it in the machine code. We do this
by replacing `objPtr(note)` with `objPtr(clone(note))`.

After specifying the parameter, we emit the actual function call using the `fnCall` instruction. The
parameters to this instruction are as follows. The first parameter is a reference to the function to
call. In our case, we get the entity for the `clone(Note)` function using `named{clone<Note>}`, and
call the `ref` function to get the reference. The second parameter is `false` since the `clone`
function is not a member function. The third parameter specifies that the function returns a
pointer. The fourth parameter then specifies that the return value of the function should be stored
in the register `ptrA`.

After calling the function we conclude the function by returning the value in `ptrA` using the
`fnRet` operation.

With these additions, we can run the program, and now it is possible to use both `print(MyNote.toS)`
and `print(named{MyNote}.note.toS)` to acquire our note. The benefit of using a function in this
manner is that it has a high likelihood of being future-proof as well, since most languages will
likely understand the `Function` entity in Storm. This is the same approach that the Syntax Language
uses to be able to interface with Basic Storm even though Basic Storm is unaware of rules and
productions. Since the `Rule` and `Production` entities inherit from the `Type` entity, they appear
as regular types to Basic Storm, just as the `NoteEntity` appears as a `Function` to Basic Storm in
this tutorial.
