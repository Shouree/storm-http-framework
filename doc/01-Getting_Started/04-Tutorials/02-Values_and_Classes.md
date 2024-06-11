Values and Classes
==================

Storm, and therefore also Basic Storm, has three kinds of types: value types, class types, and actor
types. This tutorial cover the first two of them: value types and class types. In particular, we
will look at the differences between value types and class types, and how to implement new types in
Basic Storm.

The code presented in this tutorial is available in the directory `root/tutorials/types` in the
Storm release. You can run it by typing `tutorials:types:main` in the Basic Storm interactive
top-loop.

Setup
-----

Before starting to write code, we need somewhere to work. For this tutorial, it is enough to create
a file with the extension `.bs` somewhere on your system. The tutorial uses the name `types.bs`, but
any name that contains only letters will work. As a starting point, add the following code to the
file:

```bs
void main() {
}
```

When you have created a file, open a terminal and change to the directory where you created the
file. You can then run it by typing:

```
storm types.bs
```

Note that based on how you have installed Storm, you might need to [modify the
command-line](md:../Running_Storm/In_the_Terminal) slightly. Also, if you named your file something
different than `types.bs`, you need to modify the name in the command accordingly.

If done correctly, Storm will find your file, notice that it contains a `main` function and execute
it. Since the `main` function does not yet do anything, Storm will immediately exit without printing
anything.


Value Types
-----------

Let's start by exploring *value types*. A value type is a type that has value semantics. This means
that value types are copied when passed as a parameter to a function, returned from a function, and
when using the assignment operator (`=`). Since value types are frequently copied, value types
should typically be kept small in size, to avoid needlessly copying large amounts of data.

To illustrate the semantics in more detail, we start by creating a value type in the file we created
earlier:

```bs
value Pair {
    Nat key;
    Str value;
}
```

So far the value type is very simple. It just contains two data members: a `Nat` named `key` and a
`Str` named `value`. We also need to write some code to actually use the `Pair`. We do this in a
function we call `valueMain`, so that we can compare the behavior to class types later on. As a
start, we simply create an instance of the value and print its `key` and `value`:

```bs
void valueMain() {
    Pair p;
    print("Key: ${p.key}");
    print("Value: ${p.value}");
}
```

We also need to add the following to `main` to actually call our new function:

```bs
void main() {
    print("-- Value ---");
    valueMain();
}
```

If we run this program, we get the following output:

```
-- Value --
Key: 0
Value: 
```

Basic Storm requires that all variables are initialized to some well-defined value. In our example,
Basic Storm helpfully generated a *default constructor* for our type. This constructory simply calls
the default constructors of all data members in the type. This is why the key is guaranteed to
contain the value 0, and why the value contains an empty string. In fact, since `Str` is a class
type, Basic Storm had to heap-allocate an instance of the `Str` class for us, since `null` is not
considered to be a valid value for variables by default.

### Constructors

If we define any constructor for a type, Basic Storm will not generate a default constructor. This
means that if we wish to require that users of our type should explicitly provide a key and a value
to our class, we can simply define a constructor that accepts two parameters and initializes the
member variables:

```bs
value Pair {
    Nat key;
    Str value;

    init(Nat key, Str value) {
        init {
            key = key;
            value = value;
        }
    }
}
```

As we can see, constructors are declared similarly to functions. However, the special name `init`
(for *initialize*) is used, and no return type is specified. A constructor is also special in the
sense that it may contain a `init` block. The `init` block in the constructor is where the object is
actually initialized. As such it is not possible to access member variables and member functions
before the `init` block, since the object does not exist at that point. The `init` block itself
contains a list of initializers. Here, we specify that we wish to initialize the `key` member
variable using the `key` variable, and the `value` member using the `value` variable. Note that
while these initializers may look ambiguous at first glance, they are in fact not. The left hand
side of the `=` sign must refer to a member variable. The right hand side is a general expression
that might refer to either the parameter or the member variable. However, since the object has not
yet been created, the parameter is the only option.

It is also possible to write the initializers as `key(key)` and `value(value)` respectively. This
syntax allows passing multiple parameters to the constructor of the member variables if desired.

Since we now have created a constructor, we have now inhibited the creation of the default
constructor. Since our constructor requires two parameters, the code we wrote that creates a `Pair`
will fail to compile with the following error:


```
@/home/storm/types.bs(140-141): Syntax error:
No appropriate constructor for types.Pair found. Can not initialize 'p'.
Expected signature: __init(types.Pair&)
```

The error message tells us that it failed to find a suitable constructor to initialize the variable
`p`. It expected to find a constructor that only accepts the implicit `this` parameter, but this
constructor no longer exists. If we wish to allow default initialization, we can of course manually
create a constructor that accepts no parameters as well:

```bsclass
init() {
    init {
        key = 1;
        value = "default";
    }
}
```

Note that we can omit the initialization of `key` and `value` if we wish. This makes Basic Storm
initialize them using their default constructor as it did in the default constructor. It is also
possible to give member variables default values in the declaration of the variable itself:

```bsclass
Nat key = 1;
Str value = "default";
```

Regardless of if we want to allow initializing a `Pair` without parameters or not, we can provide
the parameters to the constructor in our `valueMain` function as follows:

```bsstmt
Pair p(5, "test");
```

### Creating a String Representation

One thing that is common to do is to make it convenient to print values. Let's see what happens by
default by adding the following to our `valueMain` function:

```bsstmt
print("Pair: ${p}");
```

In this case we get the following error message:

```
@/home/storm/types.bs(266-272): Syntax error:
Can not convert types.Pair& to string by calling 'toS'.
```

As we can see, the system complains that it does not know how to create a string representation of
our `Pair` type. The best way to do this is by defining a `toS` member function that accepts a
`StrBuf`. This makes Storm able to generate a `toS` function and a `<<` operator for the type
automatically. As such, we do this by defining a member function as follows:

```bs
void toS(StrBuf to) {
    to << "{ key: " << key << ", value: " << value << " }";
}
```

Note that we do not need an explicit `return` statement since the value of the last expression is
returned automatically, and in this case it returns `to` exactly as we would like it to. After
defining this function, we can run our program to get the following output:

```
-- Value --
Key: 5
Value: test
Pair: { key: 5, value: test }
```

### Value Semantics

To illustrate the implications of the value semantics mentioned earlier, let's expand the code in
our `valueMain` function a bit:

```bs
void valueMain() {
    Pair p(5, "test");
    print("Pair: ${p}");

    Pair copy = p;
    copy.key += 5;

    print("Copied: ${copy}");
    print("Original: ${p}");
}
```

In the new code, we first make a copy of `p` into a new variable using the `=` operator. This is
equivalent to using the `=` operator to assign to existing variables. Then, the program adds 5 to
`key` and prints the values of the two variables. Since the value semantics dictates that
assignments create a copy, the modifications to `copy` are not visible in `p`:

```
-- Value --
Pair: { key: 5, value: test }
Copied: { key: 10, value: test }
Original: { key: 5, value: test }
```

Similarly, value types are copied when they are passed to functions. We can verify this by creating
a function and calling it:

```bs
void valueFunction(Pair p) {
    p.key += 10;
    print("Inside the function: ${p}");
}

void valueMain() {
    Pair p(5, "test");
    print("Pair: ${p}");

    Pair copy = p;
    copy.key += 5;

    print("Copied: ${copy}");
    print("Original: ${p}");

    valueFunction(p);
    print("After calling the function: ${p}");
}
```

Again, even though the function modifies the parameter, the changes are *not* visible outside of the
function since the `Pair` is copied when passed as a parameter to the function. As such, we get the
following output as we would expect:

```
-- Value --
Pair: { key: 5, value: test }
Copied: { key: 10, value: test }
Original: { key: 5, value: test }
Inside the function: { key: 15, value: test }
After calling the function: { key: 5, value: test }
```

There is one exception to this behavior: the implicit `this` parameter of member functions. This
exception exists to allow member functions in a value type to modify the contents of the type. For
example, we can write a member function `add` to increase the value of `key`:

```bsclass
void add(Nat toAdd) {
    key += toAdd;
}
```

We can verify that it works as we would expect by replacing the use of the `+=` operator with a call
to the `add` function:

```bs
void valueFunction(Pair p) {
    p.add(10);
    print("Inside the function: ${p}");
}

void valueMain() {
    Pair p(5, "test");
    print("Pair: ${p}");

    Pair copy = p;
    copy.add(5);

    print("Copied: ${copy}");
    print("Original: ${p}");

    valueFunction(p);
    print("After calling the function: ${p}");
}
```


Class Types
-----------

Objects of class types are always allocated on the heap, and are managed by the garbage collector.
As such, variables of a class type is just a pointer to memory on the heap. This means that class
types have *reference semantics*. This means that assignments of class types simply make a copy of
the pointer. The same is true when passing class types to and from functions. This means that for
class types, any changes made to an object by a function are generally visible to the caller.

This does, of course, require some care. However, the fact that the system does not need to copy
class types makes it cheap to pass them around, even if they are large objects. The pointer
indirection also allows a pointer to a type `T` to actually refer to an object that is a subtype of
`T`. This opens up for many common OOP techniques like inheritance, polymorphism, and dynamic
dispatch.

To illustrate this, let's start by implementing a `ClassPair` class like the one for value types
before. The only difference in the implementation so far is that we use the keyword `class` instead
of `value` to define the type:

```bs
class ClassPair {
    Nat key;
    Str value;

    init(Nat key, Str value) {
        init {
            key = key;
            value = value;
        }
    }

    void add(Nat toAdd) {
        key += toAdd;
    }
}
```

To test our implementation, we define the functions `classMain` and `classFunction` as below:

```bs
void classFunction(ClassPair p) {
    p.add(10);
}

void classMain() {
    ClassPair p(5, "test");
    print("Pair: ${p}");

    ClassPair copy = p;
    copy.add(5);

    print("Copied: ${copy}");
    print("Original: ${p}");

    classFunction(p);
    print("After calling function: ${p}");
}
```

Finally, we call `classMain` from the `main` function so that the code is actually executed:

```bs
void main() {
    print("-- Value --");
    valueMain();

    print("-- Class --");
    classMain();
}
```

If we run the code now, we get the following output:

```
-- Class --
Pair: ClassPair @0x00007F891EF5A1D8
Copied: ClassPair @0x00007F891EF5A1D8
Original: ClassPair @0x00007F891EF5A1D8
After calling function: ClassPair @0x00007F891EF5A1D8
```

This is not too helpful. Since we have not defined a string representation yet, we get the default
one provided by the `Object` class. This implementation simply prints the name of the class and its
address in memory. We can, however, see one thing from the output: all printed objects are actually
the *same object* since they have the same address. We can already at this point conclude that the
behavior will be different from the version that uses value types.

To define the string representation for classes, we simply override the `toS(StrBuf)` function. This
makes both the `toS` function and the `<<` operator work as expected, as they both call the
`toS(StrBuf)` function eventually. We do this by adding the following definition to the `ClassPair`
class:

```bsclass
void toS(StrBuf to) : override {
    to << "{ key: " << key << ", value: " << value << " }";
}
```

After adding this definition, we get output that is easier to read:

```
-- Class --
Pair: { key: 5, value: test }
Copied: { key: 10, value: test }
Original: { key: 10, value: test }
After calling function: { key: 20, value: test }
```

From this output we can indeed see that the result is different from the value types. Since
assignments and function calls have reference semantics, there is only ever one instance of the
`ClassPair` type. Therefore, the modifications done through the `copy` variable and in the
`classFunction` function are both visible through the original `p` variable. This is because all
variables refer to the same piece of memory as we saw before.

We can verify this by comparing the identities of the objects in `p` and `copy`. For class types,
the `==` operator is used to compare objects semantically (which allows using `==` for strings
safely). The reason for this will become clear when we consider the semantics of class types in the
presence of threads. Basic Storm does, however, provide the operator `is` to compare object
identities. We can use it in the `classMain` function as follows:

```bs
void classMain() {
    ClassPair p(5, "test");
    print("Pair: ${p}");

    ClassPair copy = p;
    copy.add(5);

    print("Copied: ${copy}");
    print("Original: ${p}");
    if (p is copy) {
        print("p and copy are the same object");
    } else {
        print("p and copy are different objects");
    }

    classFunction(p);
    print("After calling function: ${p}");
}
```

As we would expect, the program prints the line `p and copy are the same object`.

Since classes have reference semantics by default, we need to explicitly make copies when we need
them. Basic Storm still generates a copy constructor for this purpose. We just need to call it. We
can do this by explicitly calling the constructor as follows:

```bsstmt
ClassPair copy = ClassPair(p);
```

We can also use parentheses when declaring the variable, as this version *always* calls a
constructor:

```bsstmt
ClassPair copy(p);
```

Regardless of which option was selected, the program should now print `p and copy are different
objects`, and the changes to the variable `copied` are no longer visible in the original:

```
-- Class --
Pair: { key: 5, value: test }
Copied: { key: 10, value: test }
Original: { key: 5, value: test }
p and copy are different objects
After calling function: { key: 15, value: test }
```

Note that the copy constructor only performs a *shallow copy*. That is, only the top-level object is
copied, and any member variables that have reference semantics are still shared between the copies.
In the case of the `ClassPair` type, the `value` member of the two instances will still refer to the
same string since `Str` is a class type. This does, however, not matter here since strings in Storm
are immutable. If we wish to create a deep copy, Storm provides a `clone` function that performs
deep copies.


Polymorphism
------------

One major benefit of class types having reference semantics is that the type of the value stored in
a variable does not need to match the exact type of the variable. Storm, like many other languages,
utilizes this to implement polymorphism with dynamic dispatch. To retain type safety, Storm allows
any variable to contain a value of either the type of the variable, or a value of a subtype of the
variable.

To illustrate this, let's define a subclass to the `ClassPair` class named `CountedPair`. The goal
is to make the subclass keep track the number of times that `key` was changed. To make it possible
to pass the `CountedPair` class to functions that accept a plain `ClassPair` value, we make
`CountedPair` inherit from `ClassPair`:

```bs
class CountedPair extends ClassPair {
    Nat count;
}
```

To test our implementation, we also define a function that uses the subclass:

```bs
void inheritanceMain() {
    CountedPair p;
    classFunction(p);
    print("After calling the function: ${p}");
}
```

Finally, we update our `main` function as well:

```bs
void main() {
    print("-- Value --");
    valueMain();

    print("-- Class --");
    classMain();

    print("-- With inheritance --");
    inheritanceMain();
}
```

If we run the program at this point we will get the following error:

```
@/home/storm/types.bs(1257-1308): Syntax error:
No constructor (__init(types.ClassPair)) found in types.ClassPair.
```

The source reference points to the `CountedPair` class in its entirety. What the error tries to tell
us is that the default constructor is unable to initialize the superclass `ClassPair`, since
`ClassPair` itself does not have a default constructor. To solve this issue, we need to define a
constructor in the `CountedPair` class that provides parameters to the superclass. In this case, it
makes sense to make the constructor of the `CountedPair` class accept parameters and simply forward
them to the superclass' constructor. We pass parameters to the superclass' constructor by providing
them as "parameters" to the `init` block as follows:

```bsclass
init(Nat key, Str value) {
    init(key, value) {
        count = 0;
    }
}
```

As a sidenote, it *is* possible to split initialization into two steps by first initializing the
superclass and then the derived class. This is typically only needed when two classes depend on each
other. For completeness, it would look as follows:

```bsclass
init(Nat key, Str value) {
    // "this" is not available here.
    super(key, value);
    // "this" is available, and of type ClassPair.
    init {
        count = 0;
    }
    // "this" is available, and of type CountedPair.
}
```

Since the constructor of `CountedPair` now requires parameters, we also need to update the
`inheritanceMain` function:

```bs
void inheritanceMain() {
    CountedPair p(5, "test");
    classFunction(p);
    print("After calling the function: ${p}");
}
```

At this point the program works, but it behaves exactly the same as if we would have used a
`ClassPair`. Let's utilize the fact that we are using a derived class by overriding some member
functions. We start by overriding `toS` to print the value of `count`:

```bsclass
void toS(StrBuf to) : override {
    super:toS(to);
    to << " (count = " << count << ")";
}
```

We maark the function with `override` to declare that we intend to override an existing function.
This is not necessary for the program to work correctly. Specifying `override`, however, will cause
Basic Storm to issue an error if the function does not override a function in a super class (e.g.
because a mistyped parameter list), and can save quite a bit of effort in debugging the code.

The overridden `toS` function first calls the super class' implementation of the `toS` function
(`super:toS(to)`) and then appends the value of the `count` value. Of course it is not necessary to
call the super class' implementation if it is not necessary. After doing this we should see the
value of `count` being outputted in addition to the previous output.

Finally, we override `add` as well to actually increase `count` whenever `add` is called. We can do
this similarly to how we added an overriding function to the `toS` function:

```bsclass
void add(Nat toAdd) : override {
    count++;
    super:add(toAdd);
}
```

After this we can run the program once again. This time we can observe that our `add` function was
called, and that it increased the value of `count` to 1. It is interesting to note that the correct
version of the `add` function was called even though `add` was called by `classFunction`, which
accepts a `ClassPair` as a parameter. Basic Storm implements this using vtable based dispatch,
meaning that there is a (small) cost associated with overriding functions in this manner. In
contrast to C++, Storm does not require annotating functions that should be possible to override in
this manner with `virtual`. Rather, Storm determines which functions need dynamic dispatch
automatically, and only incurs the cost when the feature is actually used.

Finally, it is worth noting that it is possible to use inheritance for value types as well. However,
inheritance is typically not very commonly useful for value types. Since value types have by-value
semantics, and are copied in assignments and function calls, a variable of a given type always
contains a value of the specified type. As such, virtual dispatch does not work for value types.


Function Call Syntax
--------------------

Basic Storm allows omitting empty parentheses when calling functions. As such, the call `x.toS()` is
equivalent to `x.toS`. It is therefore the programmer's choice whether or not to add an empty pair
of parentheses to remind the reader of the code that a function is called.

While this might seem strange at first, it can be useful when refactoring code as it allows
seamlessly transitioning from member variables into getters and setters. This is perhaps most
clearly illustrated with an example.

We previously created the class `CountedPair` with the goal of tracking changes to the `key`
variable in the parent class. This works well if `key` is only ever changed by calling the `add`
function. However, consider the following example:

```bs
void refactorMain() {
    CountedPair p(1, "test");
    p.key = 2 * p.key + 1;
    print("Count is now: ${p.count}");
}
```

Again, we also modify `main` to run the new function:

```bs
void main() {
    print("-- Value --");
    valueMain();

    print("-- Class --");
    classMain();

    print("-- With inheritance --");
    inheritanceMain();

    print("-- Refactor --");
    refactorMain();
}
```

If we run the program att this point, it prints that `count` is zero, even though we modified `key`.
Of course this is because `key` is a public member variable that any part of the program can modify
freely. As such, we are not currently able to modify the `CountedPair` class to update `count`
whenever `key` is modified. For reasons like this, some languages encourage creating getter and
setter functions preemptively in such cases. However, Basic Storm provides an alternative path
forward. Since it is possible to call functions without parentheses, we can simply re-define `key`
to be a member function that retrieves the value:

```bs
class ClassPair {
	private Nat myKey;
	Str value;

	init(Nat key, Str value) {
		init {
			myKey = key;
			value = value;
		}
	}

    Nat key() { myKey; }

	void add(Nat toAdd) {
		myKey += toAdd;
	}

	void toS(StrBuf to) {
		to << "{ key: " << myKey << ", value: " << value << " }";
	}
}
```

At this point we are almost done. However, if we run the program now, Basic Storm will point out a
difference in the interface:

```
@/home/storm/types.bs(1743-1744): Syntax error:
Unable to assign a core.Nat to the value core.Nat. It is only possible to
assign to references (such as variables), and if an assignment function is available.
```

Basic Storm rightfully points out that it does not make sense to assign to the return value of a
function (as we do in the `refactorMain` function). It does, however, suggest that an "assignment
function" might solve our problem. An assignment function is a setter function that has been marked
with the keyword `assign`. This makes Basic Storm consider them when a variable is being assigned
to. In our case, we can define an assignment function for `key` as follows:

```bsclass
assign key(Nat newKey) {
    myKey = newKey;
}
```

Since the function is marked with `assign`, the function may be called instead of an assignment. In
our case, the line:

```bsstmt
p.key = 2 * p.key + 1;
```

Will be interpreted as:

```bsstmt
p.key(2 * p.key() + 1);
```

This is exactly what we would have to do when using a getter and a setter. As such, if we run the
program now, it works as expected except for the fact that `count` is still zero at the end of the
program. Nowever, since we now have an assignment function, we can override it in the `CountedPair`
class to get the desired behavior:

```bsclass
assign key(Nat newKey) : override {
    count++;
    super:key(newKey);
}
```

With this change, the program prints 1 as we would expect. Note, however, that we were able to
perform this refactoring by *only* modifying the two classes. We did *not* have to change the code
that used the classes at any point. In other words, we were able to "promote" the public member
variable `key` into a setter and a getter function, without changing the interface of the class.

This approach can also be used to implement read-only variables: a read-only variable can simply be
implemented as a private variable with only a get function.


Uniform Call Syntax
-------------------

Another feature of Basic Storm that might appear strange at first is that Basic Storm implements
what is sometimes called "uniform call syntax". This means that the two function calls `a.fn(b)` and
`f(a, b)` are equivalent (except in cases where `fn` is both a member and a free function, then
`a.fn(b)` prefers the member function while `fn(a, b)` prefers the free function).

This functionality makes it possible to extend the functionality of classes in other packages, while
making the extensions appear as if they were present in the extended class from the beginning. To
illustrate this, let's assume that we are writing a piece of code where we need to count how many
times a particular character occurs in a string. We can not modify the `Str` class, so we simply
define a free function to do the work for us:

```bs
Nat count(Str s, Char ch) {
    Nat result = 0;
    for (c in s) {
        if (c == ch)
            result++;
    }
    return result;
}
```

Since this is a free function, we can of course call it as:

```bsstmt
Nat c = count("hello", 'l');
print("Count: ${c}");
```

However, due to uniform call syntax, and the fact that the first parameter is `Str`, we can also
call the function as follows:

```bsstmt
Nat c = "hello".count('l');
print("Count: ${c}");
```

While the change is fairly small, the second way of calling `count` looks as if `count` is a proper
member of the `Str` class, even if our `count` function is not even in the same package as the `Str`
class.

It is worth noting that it is possible to name the first parameter `this` as we have seen
previously. This makes it possible to access members of the parameter more conveniently. For
example, if we wished to create a `reset` function for our `ClassPair` class, we could implement it
as follows:

```bs
void reset(ClassPair this) {
    key = 0;
}
```

Note that we do *not* need to write `this.key = 0` to access the member of the class, since the
parameter is named `this`. A parameter named `this` is, however, not special in any other ways. In
particular, Storm does not perform dynamic dispatch on free functions, and value types are copied as
usual.

The fact that Storm does not perform dynamic dispatch means that if we defined another `reset`
function for `CountedPair` as follows:

```bs
void reset(CountedPair this) {
    key = 0;
    count = 0;
}
```

Then, the following code will still call the implementation that accepts a `ClassPair` and thereby
not reset `count`. This is because the overload resolution only considers the static types of all
parameters, and dynamic dispatch is not performed for non-members:

```bsstmt
CountedPair counted(0, "");
ClassPair p = counted;
p.reset(); // Calls the "ClassPair" version.
```

Making `reset` into a member function will of course make the code work as we would expect, since
Storm would use dynamic dispatch in that case.

The fact that value types are copied as usual for free functions means that we can not implement the
`reset` function for the value-type `Pair`. For example:

```bs
void reset(Pair this) {
    key = 0;
}
```

```bsstmt
Pair p(5, "");
p.reset();
print(p.key.toS()); // prints 5
```

This code would compile and run. However, since the variable `p` is copied into the parameter `this`
of the function `reset`, the changes made inside `reset` are not visible to the caller, and
therefore the `print` statement will still print 5.
