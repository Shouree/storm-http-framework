Syntax Transforms
=================

As described in the [syntax](md://BNF_Syntax), the syntax language does not only describe the
grammar used by the parser, but also describes how to extract information from the parse tree in
order to build an abstract syntax tree in a representation suitable to the specific language. This
allows language implementations to delegate much of the tedious work of constructing the abstract
syntax tree to the parser and the syntax transforms with simple declarations in the syntax language.

In this section, we will first look at the *parse tree* produced by the parser, and then how the
declarations in the grammar allow the parse tree to be transformed into a syntax tree.

The parse tree
---------------

When the parser in Storm is used to parse an input string, it produces a *parse tree*. This
representation closely resembles the structure of the grammar used when parsing, and creating the
parse tree does not require executing any of the functions specified in the grammar.

Each node in the parse tree is represented by a class associated to the production that was matched
in order to create the node. In fact, each rule and production is represented as a type inside
Storm, meaning that other languages is able to access these types as if they were any other type in
Storm. The types associated with each production inherit from the type associated with the rule on
the left hand side in the production, so that the idea that any of the productions associated with a
rule can be used interchangeably is captured by the type system. To illustrate this, consider the
following example. Note, the `= <name>` part at the end of productions is used to give the
productions a name. If omitted, an anonymous identifier is generated for them, and this identifier
is generally neither stable nor accessible by other languages. When using syntax transformations, it
is generally not required to name all productions, but it is done consistently here to show which
class corresponds to which production.

```
void A();
A : "a" = A1;
A : "b" = A2;
```

The syntax above generates the following types:

```
class A extends lang:bnf:Node {}

class A1 extends A {}

class A2 extends A {}
```

So far, this is not very interesting as the classes do not contain any data, aside from
`lang.bnf.Node`, which contains a member named `pos` that stores the location of the match in the
string. The classes are selectively populated with data based on which tokens were annotated in the
grammar. Each token followed by a name is added as a data member in the corresponding production
class. If the `->` syntax was used (meaning: call a function with this thing as a parameter), a data
member is also generated, but an index is added to the data member, since multiple calls to the same
function is allowed. To illustrate this, consider the following example:

```
void A();
A : "a" a - B b = A1;

// Note: This will not compile as a "result" part is required for the -> syntax. It is merely
// used to show which data members will be present in the classes below.
A : B -> b - B -> b = A2;

void B();
B : "[0-9]+" nr = B1;
```

This generates the following types with the following data members. Note that the arrow syntax will
not compile, as it requires a *result* part of the production, which we will talk about later.

```
class A extends lang:bnf:Node {}

class A1 extends A {
    core:lang:SStr a;
    B b;
}

class A2 extends A {
    B b0;
    B b1;
}

class B extends lang:bnf:Node {}

class B1 extends B {
    core:lang:SStr nr;
}
```

Note: The class `SStr` is a class used for storing a syntax node containing a single string. It has
a member `transform` that allows transforming it into a regular string, which can be seen later in
this section.

In case one or more tokens are enclosed in a repetition syntax, the types of the data members are
adjusted accordingly. If the part is repeated zero or one times, an optional type is used. If more
than one repetitions are allowed, an array is used. For example, assume that the following
additional productions were added to the previous example:

```
A : (B b)* = A3;
A : (B b)? = A4;
```

They will generate the following types:

```
class A3 extends A {
    Array<B> b;
}

class A4 extends A {
    Maybe<B> b;
}
```

The fact that each rule and production in the syntax is represented by a type in this manner, and
that these types are used to represent the parse tree emitted by the parser, we can see that it is
possible for other languages in Storm to inspect and modify the parse trees as appropriate. For
example, in Basic Storm, it is possible to examine which production was matched by using the `as`
operator as follows:

```
B findB(A root) {
    if (root as A1) {
        // 'root' is now of type 'A1'.
        return root.b;
    } else if (root as A2) {
        // 'root' is now of type 'A2'.
	return root.b0;
    } else {
        throw ...;
    }
}
```

It is also possible to modify the parse tree in a similar fashion since as far as other languages
know, it is just a set of regular objects. It is even possible to replace parts of the parse tree
with custom subclasses of the rule subclasses. In that case, take note of the constraints imposed by
the *Transforms* section below.

The `Node` class also contains a few functions for conveniently traversing the hierarchy in a
generic fashion. `children` gives all direct children of the current node, `allChildren` traverses
the entire tree recursively. `allChildren` optionally takes a type-parameter to only extract nodes
of a particular type.


Transforms
-----------

Even though it is possible to traverse the types generated by the parser described above, it quickly
becomes tedious. Therefore, the system also provides the ability to generate such code
automatically. The idea is to provide a member function, `transform`, in each generated class that
behaves according to the declarations in the productions. The class corresponding to a rule declares
an abstract function with the parameters and return value declared in the rule declaration. The
classes corresponding to productions then override this function with the specific behaviour for
that production. This is perhaps best illustrated with an example:

```
Int Expr();

Expr => nr : Number nr = Simple;
Expr => +(a, b) : Expr a - "\+" - Number b = Addition;
Expr => -(a, b) : Expr a - "-" - Number b = Subtraction;

Int Number();

Number => asInt(nr) : "[0-9]+" = Nr;
```

This will, as we saw earlier, generate the following classes:

```
class Expr extends lang:bnf:Node {
    Int transform() : abstract;
}

class Simple extends Expr {
    Number nr;

    Int transform() {
        Int nr = this.nr.transform();
	Int me = nr;
	return me;
    }
}

class Addition extends Expr {
    Expr a;
    Number b;

    Int transform() {
        Int a = this.a.transform();
	Int b = this.b.transform();
	Int me = +(a, b); // Equivalent to "a + b"
	return me;
    }
}

class Subtraction extends Expr {
    Expr a;
    Number b;

    Int transform() {
        Int a = this.a.transform();
	Int b = this.b.transform();
	Int me = -(a, b); // Equivalent to "a - b"
	return me;
    }
}

class Number extends lang:bnf:Node {
    Int transform() : abstract;
}

class Nr extends Expr {
    core:lang:SStr nr;

    Int transform() {
        Str nr = this.nr.transform();
	Int me = asInt(nr);
	return me;
    }
}
```

The implementations of `transform` are deliberately written as they are implemented in the syntax
language to clarify the process. As we can see, the implementation first transforms all nodes
required to call the function declared in the production, and then assigns the result of that
expression to the variable `me`. After doing that, the implementation attempts to transform any
remaining nodes in the order they appeared in the input text if they could have usable side effects
(i.e. if they take parameters, or if the result is used somewhere). Then, any tokens using the arrow
syntax (`->`) are called, which is illustrated in the example below that removes all `a`s from the
input:

```
StrBuf RemoveA();
RemoveA => StrBuf() : ("[^a]*" -> add - "a")* - "[^a]*" -> add = Impl;
```

Which will generate the following code:

```
class RemoveA extends lang:bnf:Node {
    StrBuf transform() : abstract;
}

class Impl extends RemoveA {
    Array<core:lang:SStr> add0;
    core:lang:SStr add1;

    StrBuf transform() {
        StrBuf me = StrBuf();
	for (Nat i = 0; i < this.add0.count; i++) {
	    Str add0 = this.add0[i].transform();
	    me.add(add0);
	}
	Str add1 = this.add1.transform();
	me.add(add1);
	return me;
    }
}
```

We can also rewrite the grammar to utilize recursion and parameters, to illustrate how that
works. In this particular case, we also use a token without binding it to a variable, wich generates
a variable with the name `<anonX>`, where `X` is an integer:

```
StrBuf RemoveA();
RemoveA => StrBuf() : RemoveImpl(me) = Impl;

void RemoveImpl(StrBuf to);
RemoveImpl => to : RemoveImpl(to) - "a" - "[^a]*" -> add = Rec;
RemoveImpl => to : "[^a]*" -> add = Base;
```

Which generates the following code:

```
class RemoveA extends lang:bnf:Node {
    StrBuf transform() : abstract;
}

class Impl extends RemoveA {
    RemoveImpl <anon0>;

    StrBuf transform() {
        StrBuf me = StrBuf();
	<anon0>.transform(me);
	return me;
    }
}

class RemoveImpl extends lang:bnf:Node {
    void transform(StrBuf to) : abstract;
}

class Rec extends RemoveImpl {
    RemoveImpl <anon0>;
    core:lang:SStr add1;

    void transform(StrBuf to) {
        StrBuf me = to;
	<anon0>.transform(to);
	Str add0 = this.add0.transform();
	me.add(add0);
    }
}

class Base extends RemoveImpl {
    core:lang:SStr add0;

    void transform(StrBuf to) {
        StrBuf me = to;
	Str add0 = this.add0.transform();
	me.add(add0);
    }
}
```

In this context, it is fairly easy to understand the implications of the `@` operator in the
grammar. If an `@` is prepended to a name in the grammar, the syntax transform will not call
`transform` on the associated syntax node before binding it to a variable or passing it to a
function. Thus, the parse tree node is used directly instead. This is useful in cases where a part
of the parse tree is to be transformed in a different context compared to the parent, when it should
be transformed at a later time, or when it has to be modified before transformation.

As noted previously, it is possible to modify the parse tree before it is transformed, even
inserting custom subclasses to the rule classes. If this is done, care must be taken to override the
`transform` function as would be done for regular production types.
