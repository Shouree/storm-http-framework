Loops
=====

Basic Storm has three different loops:

- The generic loop:

  ```bsstmt:placeholders
  do {
  } while (<condition>) {
  }
  ```

- The for loop:

  ```bsstmt
  for (Nat i = 0; i < count; i++) {
  }
  ```

- The range loop:

  ```bsstmt:placeholders
  for (x in <container>) {
  }
  ```

While the loops are technically expressions, none of them return anything other than void (since
they may execute zero times). It is therefore not very useful to use them as expressions. This might
change in the future.

As in C++, it is possible to use `break` and `continue` inside any of the loops. `break` exits the
loop, while `continue` aborts the current loop iterations and continues to the next. Custom loops
with support for `break` and `continue` should inherit from the `lang:bs:Breakable` class.


The Generic Loop
----------------

The generic loop is a generalization of the `while` and the `do-while` loops in C++ and Java. It has
the following generic form:

```bsstmt:placeholders
do {
    <do expression>;
} while (<condition>) {
    <while expression>;
}
```

All parts are optional. It is possible to select a sequence of one or more parts of the loop to form
the following different loops. The first one is the traditional while loop:

```bsstmt:placeholders
while (<condition>) {
}
```

And the traditional do-while loop:

```bsstmt:placeholders
do {
} while (<condition>);
```

But also an infinite loop like so:

```bsstmt:placeholders
do {
}
```

And a loop with the condition in the middle of an iteration:

```bsstmt:placeholders
do {
} while (<condition>) {
}
```

The last form is useful when checking input. For example, the following type of code is sometimes
common in C++:

```bs
int inputPositive() {
    int result = 0;
    while (true) {
        cout << "Enter a positive integer: ";
        cin >> result;
        if (result > 0)
            break;
        cout << "Incorrect input. Only positive integers!" << endl;
    }
    return result;
}
```

To avoid repeating parts of the code, the programmer has opted to use `break` to exit the loop
midway. In Basic Storm, one can write the same logic as follows:

```bs
Int inputPositive() {
    do {
        print("Enter a positive integer: ");
        Int result = readInt(); // Note: for illustration, not present in the standard library.
    } while (result <= 0) {
        print("Not correct. ${result} is not positive!");
    }
    return result;
}
```

One observation from the example above is that even though the body of the loop is split into two
blocks, variables from the first block are visible in the condition and in the second block.


The For Loop
------------

The for loop looks like in C++ and Java:

```bsstmt:placeholders
for (<initializer>; <condition>; <update>) {
}
```

Since variable declarations are expressions in Basic Storm, no special consideration needs to be
made for the `<initializer>` portion of the loop. It is just an expression.

The semantics are equivalent to:

```bsstmt:placeholders
{
    <initializer>;
    while (<condition>) {
        // Loop body.
        <update>;
    }
}
```

With the exception that `continue` executes `<update>` before continuing to the next iteration. The
above loop expansion would not call `<update>` when `continue` is used.


The Range Loop
--------------

The final loop (which is actually implemented entirely in Basic Storm itself) is the range loop. It
is dedicated to iterating through a container using any of the
[iterators](md:Language_Reference/Iterators) available in the standard library. It has the following
form:

```bsstmt:placeholders
for (<key>, <value> in <container>) {
}
```

Here, `<key>` and `<value>` are two identifiers that determine the names of the variables used for
the current element in the loop body. `<key>` can be omitted, and must be omitted for iterators that
provide no notion of a key. `<container>` is an arbitrary expression that evaluates to a container.

The range loop supports three types of iteration strategies. They are all illustrated in the context
of the following loop, where `<container>` is some expression that produces a container:

```bsstmt:placeholders
for (k, v in <container>) {
    print("Key: ${k}");
    print("Val: ${v}");
}
```

The tree types are as follows. They are attempted in the following order:

1. If `begin` and `end` are available, then iterators are used. The loop is transformed into the
   following:

   ```bsstmt:placeholders
   {
       var _container = <container>;
       var _end = container.end();
       for (var _at = container.begin(); _at != _end; ++_at) {
           var k = _at.k();
           var v = _at.v();

           print("Key: ${k}");
           print("Val: ${v}");
       }
   }
   ```

   Note that the key is only useable if the iterators of the container provide the `k` member.

2. If `iter` is available, and returns a type with a member `next`, then the loop is transformed
   into the following:

   ```bsstmt:placeholders
   {
       var _iter = container.iter();
       while (v = container.next) {
           print("Val: ${v}");
       }
   }
   ```

   Note that this iteration strategy does not support keys.

3. Finally, if `count` and the `[]`-operator are available, then the loop is transformed into the
   following:

   ```bsstmt:placeholders
   {
       var _container = container; // To only evaluate 'container' once.
       var _count = container.count;
       for (Nat k = 0; k < _count; ++k) {
           var v = _container[k];

           print("Key: ${k}");
           print("Val: ${v}");
       }
   }
   ```

   Note that the key is marked as constant after the loop header in this transform. This is,
   however, not possible to express in source code currently.
