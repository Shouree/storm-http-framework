Geometry Library
================

Storm provides a number of types for 2D and 3D geometry in its standard library. These are provided
to make it easier for different graphics libraries to interact with each other, even if parts of
them are implemented in C++.

All of these types and functions are in the `core.geometry` package. All their members treat the
values as if they were immutable. That is, it is possible to modify the contents of all values
except for `Angle` by assigning to their members. All member functions do, however, not modify the
data structure, but instead return an updated copy. This typically makes it easier to chain
operations conveniently without unintended side-effects.


Point
-----

The value [stormname:core.geometry.Point] stores a 2-dimensional point expressed by 2
[stormname:core.Float]s named `x` and `y`. The implementation assumes that points appear in a
coordinate system where positive `x` is towards the right, and positive `y` is downwards (as is
typical for 2D computer graphics).

The `Point` type has the expected arithmetic operations as overloaded operators. Namely, addition,
subtraction, multiplication with a scalar, and division by a scalar. The `*` operator for two points
is overloaded as the dot product. It also has the following members:

```stormdoc
@core.geometry.Point
- .tangent()
- .lengthSq()
- .length()
- .taxiLength()
- .normalized()
```

The following free functions are also available:

```stormdoc
@core.geometry
- .abs(Point)
- .project(Point, Point, Point)
```


Size
----

The value [stormname:core.geometry.Size] stores a 2-dimensional size expressed by 2
[stormname:core.Float]s named `w` and `h` (for width and height). Since the `Size` class is expected
to represent a size, both of `w` and `h` are expected to be non-negative.

Overall, the size is convertible to a `Point`, and follows the same basic interface. However, due to
the expectation that it contains a size, the interface is smaller. Apart from arithmetic operators,
it has the following members:

```stormdoc
@core.geometry.Size
- .__init()
- .__init(Float)
- .__init(Float, Float)
- .__init(Point)
- .valid()
- .min(Size)
- .max(Size)
```

Apart from the members, the following free functions are available:

```stormdoc
@core.geometry
- .max(Size)
- .min(Size)
- .abs(Size)
- .center(Size)
```

Rect
----

The value [stormname:core.geometry.Rect] stores a 2-dimensional rectangle, expressed as two points,
`p0` and `p1`. The first point `p0` is the top-right corner of the rectangle, and `p1` is the
bottom-right corner of the point. The member functions of `Rect` considers the rectangle to contain
all points except up to, but not including `p1`. The point class also contains the function `size`
that computes the size of the rectangle. The `size` function also has an assign variant, so it is
possible to treat the point as if it cointained a point `p0` for the top-left corner, and a `size`
instead of two points.

The `Rect` type has overloads the `+` and `-` operators where the right hand side is a `Point` to
translate a rectangle based on the coordinates in the point.

The `Rect` type has the following parameters:

```stormdoc
@core.geometry.Rect
- .__init()
- .__init(Size)
- .__init(Point, Size)
- .__init(Point, Point)
- .__init(Float, Float, Float, Float)
- .normalized()
- .contains(*)
- .intersects(*)
- .sized(*)
- .center()
- .at(*)
- .include(*)
- .scaled(*)
- .shrink(*)
- .grow(*)
```

The following free functions are also available:

```stormdoc
@core.geometry
- .inside(Point, Rect)
```

Angle
-----

The value [stormname:core.geometry.Angle] represents an angle in some unit (internally, radians is
used, but this may change). The value overloads the operators `+` and `-` for adding and subtracting
angles, and the operators `*` and `/` for multiplication and division by a scalar. While the `Angle`
type does not contain any constructors, the functions `deg` and `rad` are provided instead.
Furthermore, Basic Storm allows creating angles using units, like so:

```bsstmt
Angle a = 90 deg;
Angle b = 2.3 rad;
Float pi = (180 deg).rad();
```

An `Angle` is not limited to contain a value between 0 and 360 degrees. However, storing large angle
values will yield a loss in precision. The member `normalized` normalizes the angle to between 0 and
360 degrees.

As mentioned above, the following functions are used to create angles from numbers in degrees or
radians:

- [stormname:core.geometry.deg(Float)]
- [stormname:core.geometry.rad(Float)]

The `Angle` class contains the following members:

```stormdoc
@core.geometry.Angle
- .normalized()
- .opposite()
- .rad()
- .deg()
```

There are also a number of free functions that utilize angles together with other parts of the
geometry package:

```stormdoc
@core.geometry
- .sin(Angle)
- .cos(Angle)
- .tan(Angle)
- .asin(Float)
- .acos(Float)
- .atan(Float)
- .angle(Point)
- .angle(Angle)
```


Transform
---------


Vector
------
