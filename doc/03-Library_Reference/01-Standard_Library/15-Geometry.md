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


Vector
------

The library also provides a type for 3-dimensional coordinates. This type is name
[stormname:core.geometry.Vector]. As with points, the type provides arithmetic operations for vector
addition, subtraction, and multiplication with scalars. It is also possible to implicitly cast a
`Point` into a `Vector` in order to make it easier to use transforms for both 2D and 3D coordinates.

The 3-dimensional `Vector` uses `*` for dot product, and `/` for cross product.

Apart from that, the interface is similar to that of `Point`, but some operations are missing due to
them having a different meaning in 3 dimensions:

```stormdoc
@core.geometry.Vector
- .lengthSq()
- .length()
- .taxiLength()
- .normalized()
```

Similarly, there are also a few free functions:

```stormdoc
@core.geometry
- .abs(Vector)
- .project(Vector, Vector, Vector)
```


Transform
---------

The library also contains the [stormname:core.geometry.Transform] class, which represents a
3-dimensional affine transform using a 4 by 4 matrix. This means that the transform can represent
arbitrary combinations of for example translation, rotation, scaling, skewing, and projections.

A transform interprets points or vectors as four-dimensional row-vectors. The fourth element, `w`,
is set to 1 for the purposes of the transform. The row vector is multiplied with the transform
matrix from the left, and the resulting four-dimensional vector is reduced to three dimensions by
dividing the `x`, `y`, and `z` coordinates by `w`. As such, when transforms are multiplied together
as `a * b * c`, then they will be applied from left to right with respect to the geometry.

The `transform` class itself has few members:

```stormdoc
@core.geometry.Transform
- .__init()
- .*(*)
- .inverted()
- .at(Nat, Nat)
```

There are, however, a number of free functions that allow transforming points and vectors:

```stormdoc
@core.geometry
- .*(Vector, Transform)
- .*(Point, Transform)
```

There are also a number of functions that create various transforms:

- [stormname:core.geometry.translate(core.geometry.Vector)]

  Creates a transform that translates points by the coordinate `v`. Also accepts `v` as a `Point` or
  a `Size`.

- [stormname:core.geometry.rotateX(core.geometry.Angle)]

  Creates a transform that rotates points around the X axis. Also available for the Y axis and the
  Z axis.

- [stormname:core.geometry.rotateX(core.geometry.Angle, core.geometry.Vector)]

  Creates a transform that rotates point around a line that extends from the point `origin` and is
  parallel to the X axis. As with the other rotate function, there are variants for the Y axis and
  the Z axis as well.

- [stormname:core.geometry.rotate(core.geometry.Angle)]

  Creates a transform that rotates points around the Z axis. This is the one rotation that is
  relevant in 2 dimensions, which is why it has a special name.

- [stormname:core.geometry.rotate(core.geometry.Angle, core.geometry.Point)]

  Creates a transform that rotates points around a line that extends from `origin` that is parallel
  to the the Z axis. This is the one rotation that is relevant in 2 dimensions, which is why it has
  a special name.

- [stormname:core.geometry.scale(core.geometry.Vector)]

  Creates a transform that scales points according to the dimensions in `scale`. Also available in
  versions that accepts `scale` as a `Float` (affects all axes), and a `Size` (affects the X and Y
  axes).

- [stormname:core.geometry.skewX(core.geometry.Angle)]

  Creates a transform that skews points along the X axis by `angle`. Also available in versions that
  skew along the Y axis and the Z axis.
