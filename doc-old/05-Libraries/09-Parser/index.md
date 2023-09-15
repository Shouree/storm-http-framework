Parser
======

The `parser` library allows pre-compiling grammar into a specialized parser. This is a useful
complement to the parser used by the compiler for several reasons:

- The system parser only executes on the `Compiler` thread, which might be unsuitable in certain needs.
- The system parser is heavily focused on extensibility. As such, the grammar is examined and
  prepared dynamically as it is needed. This cost might be unsuitable in situations where the
  language is static and known ahead of time.
- To support a wide variety of grammars, the system parser employs powerful but slow parsing
  methods (e.g. GLR parsing). In cases where the grammar is known beforehand, much cheaper
  parsers can be used instead.
- The system parser only supports strings, and thus disallows parsing binary data.

The parser library addresses these shortcomings by allowing grammar to be pre-compiled, and then
executed as regular Storm functions. The grammar may still be specified using the regular syntax
language, and can thus be re-used between the system parser and parsers from the parser library.

This library is usable as a syntax extension to Basic Storm as follows:

```
myParser : parser(recursive descent) {
    start = Rule;
    // ...
}
```

This creates a parser function, `myParser` that parses the specified language. Grammar can be
declared inside the brackets or in other `.bnf` files as usual. Multiple parsers are supported by
the syntax, but currently only recursive descent parsers are implemented.

For more details, see the documentation for the `parser` package (either `help parser` or look in
the file `root/parser/README`).
