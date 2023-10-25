Time
====

The Storm standard library contains basic functionality for measuring time. This API is currently
aimed at measuring elapsed time, not to handle clocks, time zones, etc.

The functionality is provided by the two types [stormname:core.Moment] and
[stormname:core.Duration]. The first, `Moment` represents a moment in time. The `Moment` is
represented as a time-stamp based on some high-resolution clock in the system. A `Duration` is then
the difference between two `Moment`s, that is a measurement of how much time has elapsed between two
points in time. Since the `Moment` has no defined anchor in time, it is usually not meaningful to
speak about the absolute value of a a moment. They are only meaningful to create `Duration`s.

Moment
------

As previously mentioned, a [stormname:core.Moment] represents some point in time as measured by a
high-resolution clock in the current system. This clock is typically not related to wall-time clock
in a meaningful way. This means that a `Moment` is good to measure time at a high precision, but it
is not a good way to communicate points in time to others, as their clock may have a different
zero-point.

The type is a value that contains a single `Long` that contains the value of the clock in
microseconds. The variable is named `v` for *value*. When created, the `Moment` class captures the
current time and stores it in itself. As such, the operation that is typically interesting for the
`Moment` class is to create it, and to do arithmetics on it with `Durations`.

It is possible to subtract two `Moment`s to create a `Duration` that represents the time elapsed
between them. It is also possible to add a `Duration` to a `Moment` to create a new `Moment` that
represents a time in the future. Similarly, `Duration`s can be subtracted from `Moment`s to
represent a time in the past. It is also possible to compare `Moments` to establish their causual
relation.


Duration
--------

The value [stormname:core.Duration] represents a difference between two `Moment`s expressed in
microseconds. As with `Moment`, this value is stored in the member `v` (for *value*). It is,
however, also accessible in different units using the one of the following functions:

```stormdoc
@core.Duration
- .inUs()
- .inMs()
- .inS()
- .inMin()
- .inH()
```

Creating a `Duration` using its default constructor creates the duration of zero. To create other
values, one of the helper functions `h`, `min`, `s`, `ms`, or `us` can be used. They create a
`Duration` initialized to the specified timespan in the specified unit. Basic Storm also allows
using these functions as units. As such, one can write:

```bsstmt
Duration x = 5 min;
sleep(2 s);
```

As with `Moment`, the `Duration` class provides many arithmetic operators for addition and
subtraction of durations. It is also possible to scale a duration with an `Int` or `Float` factor
(with `*` and `/`), computing the ratio between two `Durations` using `/`, repeating a `Duration`
using `%`, finding minimum and maximum values using `min` and `max`, and comparing them. All of
these operators work as one would expect when representing a `Duration` as a number.

The library also provides the ability to sleep for a specified duration:

```stormdoc
- core.sleep(core.Duration)
```