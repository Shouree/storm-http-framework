Generators
==========

Generators are a low-level construct in Basic Storm that allows dynamic generation of functions,
types, or other things. A generator is a function that is called whenever Storm attempts to look up
a name that does not yet exist. The generator then has the opportunity to generate a matching entity
dynamically. It is thus possible to implement C++-style templates with generators.

A generator is associated with a name. However, unlike other entities in the name tree, a generator
does not have a set of parameters associated with it. Instead, the generator gets access to the
parameters that the system is looking for, and it has the chance to generate a suitable entity.

*Note:* The generators are implemented in the package: `lang:bs:macro`.

A simple generator that generates a function that adds two numbers in Basic Storm looks as follows:

```bs
use lang:bs:macro;

genAdd : generate(params) {
    if (params.count != 2)
        return null;

    if (params[0] != params[1])
        return null;

    Value t = params[0];

    BSTreeFn fn(t, SStr("genAdd"), params, ["a", "b"], null);

    Scope scope = rootScope.child(t.getType());

    FnBody body(fn, scope);
    fn.body(body);

    Actual actual;
    actual.add(namedExpr(body, SStr("a"), Actual()));
    actual.add(namedExpr(body, SStr("b"), Actual()));

    body.add(namedExpr(body, SStr("+"), actual));

    return fn;
}
```

As can be seen from the example, a generator is a function that receives a single parameter (named
`params` above). This parameter has the type `Array<Value>` and represents the params of the name
part sought after. The function then validates that exactly two parameters were passed, and that
they are the same. If not, `null` is returned to indicate that the parameter list was not
applicable. Otherwise, a `Named` is generated (a function in this case) and returned.
