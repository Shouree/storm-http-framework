Generators
============

Generators are a low-level construct in Basic Storm that allows dynamic generation of functions,
types or similar things. This construct is the foundation for templates in Basic Storm.

A generator is a function that is called when the system tries to look up a name which does not yet
exist. Generators themselves have a name associated with them, but unlike functions, generators do
not have any parameters associated to them. Instead, the system asks the generator if it can provide
a `Named` with the given parameters.

A simple generator in Basic Storm looks like this:
```
genAdd: generate(params) {
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

This generator generates a function named `genAdd` that adds two parameters of the same type. The
declaration (`genAdd: generate(params)`) indicates that we're declaring a generator named `genAdd`
that receives the parameters in the local variable `params`. This declaration is more or less
equivalent to the function declaration `Named? <some name>(Value[] params)`. The generator is then
expected to return a `Named` object that follows the specification asked for. It is acceptable to
alter `params` by removing references from zero or more parameters, but nothing else. Returning
`null` means that the combination of parameters supplied is not supported by this generator.
