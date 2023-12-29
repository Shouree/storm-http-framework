Values and Classes
==================

Storm, and therefore also Basic Storm, has three kinds of types: value types, class types, and actor
types. This tutorial cover the first two of them: value types and class types. In particular, we
will look at the differences between value types and class types, and how to implement new types in
Basic Storm.

The code presented in this tutorial is available in the directory `root/tutorials/types` in the
release of Storm. You can run it by typing `tutorials:types:main` in the Basic Storm interactive
top-loop.

Setup
-----

Before starting to write code, we need somewhere to work. For this tutorial, it is enough to create
a file with the extension `.bs` somewhere on your system. The tutorial uses the name `types.bs`,
but any name that contains only letters works. As a starting point, add the following code to the
file:

```bs
void main() {
}
```

When you have created a file, open a terminal and change to the directory where you created the
file. You can instruct Storm to run the file by typing:

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
our `Pair` type. The best way to do this is to define the `<<` operator for the `StrBuf` type. This
makes Storm able to generate a `toS` function for us automatically. We do this by defining a free
function (i.e. outside of the `Pair` type) named `<<` as follows:

```bs
StrBuf <<(StrBuf to, Pair p) {
    to << "{ key: " << p.key << ", value: " << p.value << " }";
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

Since the `<<` function is not a member of the `Pair` class, we need to access `key` and `value` by
typing `p.key` and `p.value`. Since we think about the `<<` function as belonging to the `Pair`
class, it would be nice if we could treat it as such. Luckily, Basic Storm allows naming parameters
of non-member functions `this` to make the function behave more like a member function. Since Basic
Storm will attempt to resolve unqualified names as `this.<name>`, even for nonmember functions, this
makes it possible to write `key` instead of `p.value`. As such, we can update the `<<` function
into:

```bs
StrBuf <<(StrBuf to, Pair this) {
    to << "{ key: " << key << ", value: " << value << " }";
}
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
------------


Function Call Syntax
--------------------

- Note that we don't need to use parens.
- Use this to refactor the class with assign functions!


Uniform Call Syntax
-------------------

- Extend a class from elsewhere (e.g. the string class)
- Note that we can use `this` as a parameter name.




Actors are covered in the next part.

- By value vs by reference
- Adding custom types to the `toS` mechanism
- Uniform Call Syntax
- parens vs. no parens (for refactoring)
