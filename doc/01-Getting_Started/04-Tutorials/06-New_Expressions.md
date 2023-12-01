A New Expression in Basic Storm
===============================


Create dir `map`, with files


```bnf
use lang.bs;

optional delimiter = SDelimiter;
required delimiter = SRequiredDelimiter;

SExpr => MapBlock(pos, block, src, name) : "map" #keyword, "(", SExpr(me), "for" #keyword, SName name, "in" #keyword ~ SExpr(block) src, ")";
```

```bs
use core:lang;
use lang:bs;
use lang:bs:macro;

class MapBlock extends Block {
	private Expr src;
	private SStr name;
	private Expr? transform;

	init(SrcPos pos, Block parent, Expr src, SStr name) {
		init(pos, parent) {
			src = src;
			name = name;
		}
	}

}
```

Test program:

```bs
use tutorials:extension:repeat;

void main() {
	Int[] values = [8, 10, 3];
	Int[] result = map(x + 2 for x in values);
	print(result.toS);
}
```

This is enough to get a type error:

```
@/home/filip/Projects/storm/root/tutorials/extension/test.bs(82-88): Syntax error: No appropriate constructor for core.Array(core.Int) found. Can not initialize result. Expected signature: __init(core.Array(core.Int), void)
```

Adding the result overload is enough to make it crash with an "abstract function called".

```
	ExprResult result() : override {
		ExprResult(Value(named{Array<Int>}));
	}
```

Start by looking at the transform?

```
	void setTransform(Expr expr) {
		Value expected(named{Int});
		if (!expected.mayStore(expr.result.type))
			throw TypeError(pos, expected, expr.result);
		transform = expr;
	}
```

The above gets us a type error so that we have to fix the scoping.

Let's implement that!

```
	void blockCode(CodeGen gen, CodeResult res) : override {
	}
```

Now we crash with a memory access error! Nice!



- Create two simple extensions to Basic Storm. One that is an expression, and one that is a
  top-level construct.

- The extension can simply be a loop construct (repeat N times). First, implement in ASM, then use
  existing AST nodes.
