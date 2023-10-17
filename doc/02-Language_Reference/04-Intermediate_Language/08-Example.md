Example
=======

This page contains an example that illustrates how to generate code in the Intermediate Language,
how to compile the listing to machine code using a function, and then how to execute the function.

```bs
use core:asm;  // For code generation.
use core:lang; // For Function, Value, DynamicCode
use lang:bs:macro; // For named{} syntax

void callFromIR(Int param) {
    print("Called from IR: ${param}");
}

Listing createListing() on Compiler {
    // Create a non-member function that returns an integer.
    Listing l(false, intDesc);

    // It accepts a single parameter:
    Var param = l.createParam(intDesc);

    // Start generating code by emitting the prolog:
    l << prolog;

    // Double the parameter to the function:
    l << mul(param, intConst(2));

    // Call 'callFromIR':
    var toCall = named{callFromIR<Int>};

    l << fnParam(intDesc, param);
    l << fnCall(toCall.ref, false);

    // Add 5 and return it.
    l << add(param, intConst(5));
    l << fnRet(param);

    print("Generated code:\n" + l.toS);
    return l;
}

void main() on Compiler {
    // Create a Function (a named entity) with a matching signature.
    Value intVal(named{Int});
    Function f(intVal, "f", [intVal]);

    // Set its parent lookup to allow it to work properly. We could
    // also add it to the name tree. This is enough to make it
    // look like it is a member of the main function.
    f.parentLookup = named{main};

    // Set the code to our generated Listing. The DynamicCode
    // compiles the listing into a Binary and associates it with
    // the RefSource inside the function.
    f.setCode(DynamicCode(createListing));

    // Extract a pointer to the function and call it.
    if (ptr = f.pointer() as Fn<Int, Int>) {
        print("Calling the generated function...");
        Int result = ptr.call(10);
        print("Done. It returned: ${result}");
    }
}
```