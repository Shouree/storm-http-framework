Visibility
==========

All rules and productions defined in a syntax file are public. There is currently no way of
modifying the visibility in the syntax language. Use declarations and delimiters only apply to the
current file.


When parsing text using the Syntax Language, only productions that are located in included packages
are considered as candidates. Exactly what is considered to be an *included package* depends on the
language. In Basic Storm, all `use`d packages are included in the parsing process. As such, it is
possible to include syntax in a package, and that syntax will only be used for parsing whenever the
library is used.

This mechanism has some implications that might be surprising at first. For example, consider the
following grammar in package `a`:

```bnf
void Start();
Start : "<" - Content - ">";

void Content();
```

And the following grammar in package `b`:

```bnf
use a;

Content : "content";
```

If a parser is created that parses the rule `Start`, then it will not match any strings since the
rule `Content` has no productions that are visible. For the production in package `b` to be usable,
it is necessary to also include package `b` in the parser.
