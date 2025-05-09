Listings
========

A listing is a list of instructions in the intermediate language that will eventually be compiled
into a function that can be executed by the CPU. The listing also stores metadata that describes the
context in which the code operates. This metadata includes:

- Parameters to the function.
- Return value from the function.
- Local variables.
- Exception handling information.
- Labels in the function.

To allow the code in the listing to refer to variables and labels in the listing, the listing
assigns each label and variable a unique name. Since the Intermediate Language is designed as a
compilation target for other languages, the names are simply integer indexes. However, because the
listing is in charge of assigning the names for variables and labels, the listing can be thought of
as representing a namespace for these names, since the created names are only meaningful in the
context of the listing that created them.

Listings are represented by the class [stormname:core.asm.Listing]. In Basic Storm, it is possible to create a
listing as follows:

```bs
use lang:bs:macro; // For named{}
use core:asm;

void main() {
    // Is the function going to be a member function?
    Bool isMember = false;

    // What is the result type of the function?
    TypeDesc resultType = named{Int}.typeDesc();

    // Create the listing:
    Listing listing(isMember, resultType);

    // Add code to the listing:
    listing << prolog();
    // ...

    print(listing.toS());
}
```

As we can see from the code above, creating a listing requires knowing the type returned from the
listing, and if it is a member function of a type or not. The reason for this is that these two
pieces of information affect the calling convention on some systems.


Labels
------

As previously mentioned, a listing acts as a namespace for labels and variables. Therefore, new and
unique names for labels are generated by the listing itself using the `label` member:

```bsstmt
Label loopStart = listing.label();
```

Labels can then be inserted into the listing using the `<<` operator. This makes the label refer to
the next instruction that is inserted:

```bsstmt
listing << loopStart;
listing << instr;
```

Blocks
------

Variables in a listing are ordered in a tree of *blocks*. These blocks work similarly to blocks in
Basic Storm. Each block is active for some subset of the instructions in the listing, and any
variables contained in the block are only accessible to those instructions. Similarly to labels,
blocks are given names in the form of integers.

The listing initially contains a root block for variables that are visible directly from after the
prolog, to the epilog. This block is accessible by calling the member `root()` in the `Listing`
object. It is possible to create new blocks by calling `createBlock` and specifying which block the
new block should be a child of. The `Listing` also contains members to traverse the tree of blocks,
and the variables in them.

Blocks are lexically scoped in the output. A block begins at a `begin <block>` instruction, and ends
at the corresponding `end <block>` instruction. The root block is considered to begin at the
`prolog` instruction and end at the `epilog` instruction.

To illustrate how blocks are created and activated, consider the example below:

```bs
use core:asm;

void main() {
     Listing l(false, voidDesc);

     // Create a block inside the root:
     Block child = l.createBlock(l.root());

     // Activate the root block.
     l << prolog();

     // Start the child block.
     l << begin(child);

     // ...

     l << end(child);

     // Function epilog and return.
     l << fnRet();
}
```


Variables and Parameters
------------------------

Each block contains a list of variables. Some of the variables in the root block may be parameters
to the function. Each variable records its data type. In this context, the data type is simply the
size of the variable, and a reference to a destructor to call whenever the variable should be
destroyed. For parameters, the system also records the type description in order to follow the
system's calling convention properly. Each variable may also be associated with additional metadata
in the form of a [stormname:core.asm.Listing.VarInfo] object. This object stores higher-level data
that can be used by debuggers (for example, [Progvis](md:/Programs/Progvis)).

When a block begins, all contained variables are initialized to zero. Similarly, all variables are
automatically destroyed whenever the block ends, or when an exception occurs. The destruction
behavior depends on whether a destructor was specified, and the options in the
[stormname:core.asm.FreeOpt] parameter the variable was created. The following options determine
when the destructor is called, assuming one is specified:

- `FreeOpt.none`: Don't call any destructors for this variable.

- `FreeOpt.exception`: Only call the destructor when an exception unwinds the stack past the block.

- `FreeOpt.blockExit`: Only call the destructor when the block is exited.

- `FreeOpt.both`: Bitwise `or` of `FreeOpt.exception` and `FreeOpt.blockExit`. Calls the destructor
  on both in case of an exception, and in case the block is exited.


There are also two options that can be used to modify the behavior of the cleanup. These can be
bitwise `or`:ed with the options above:

- `FreeOpt.freePtr`: When freeing the variable, pass a pointer to the variable to the destructor
  rather than a copy of the value itself. This is typically desirable for types larger than 8 bytes.

- `FreeOpt.inactive`: Consider the variable to be *inactive* initially. The system will not execute
  destructors for any inactive variables until they are *activated* using the `activate`
  pseudo-operation. This is useful when it is important to ensure that a destructor is not executed
  until an object has been initialized properly. Since it is possible to access an inactive
  variable, it is possible to pass a pointer to it to a constructor, and only activate it when the
  constructor has finished initializing the variable.

  Note that activations are lexical in nature. All instructions before a variable has been activated
  are considered locations where the variable is inactive, and it is considered to be active in all
  locations after. As such, it is not possible to conditionally execute the `activate` instruction.

The following example illustrates how variables and parameters can be created and used:

```bs
use core:asm;

void main() {
    Listing l(false, voidDesc);

    // Create an integer parameter. Passing a type description (here, for an integer)
    // generally does the right thing regarding cleanup.
    Var param1 = l.createParam(intDesc);

    // Create an integer variable in the root block. Again, passing
    // a type description generally does the right thing regarding cleanup.
    Var local1 = l.createVar(l.root, intDesc);


    // Prolog, initializes the root block.
    l << prolog();

    // Copy the parameter to the local variable.
    l << mov(local1, param1);

    // End.
    l << fnRet();
}
```