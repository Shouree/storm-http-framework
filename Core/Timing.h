#pragma once

namespace storm {
	STORM_PKG(core);

	class Str;

	/**
	 * Basic time api for measuring short intervals.
	 */

	/**
	 * Represents a specific point in time.
	 */
	class Moment {
		STORM_VALUE;
	public:
		// Create a representation of *now*
		STORM_CTOR Moment();

		// Inernal use, create with specific value.
		Moment(Long v);

		// The time value, measured in us from some point in time.
		Long v;

		// To string.
		void STORM_FN toS(StrBuf *to) const;
	};

	/**
	 * Represents a difference between two points in time.
	 */
	class Duration {
		STORM_VALUE;
	public:
		// Zero duration.
		STORM_CTOR Duration();

		// Create a duration with a time in us.
		Duration(Long t);

		// The actual value, in us.
		Long v;

		// Get the duration in microseconds.
		inline Long STORM_FN inUs() const { return Long(v); }

		// Get the duration in milliseconds.
		inline Long STORM_FN inMs() const { return Long(v / 1000); }

		// Get the duration in seconds.
		inline Long STORM_FN inS() const { return Long(v / 1000000); }

		// Get the duration in minutes.
		inline Long STORM_FN inMin() const { return inS() / 60; }

		// Get the duration in hours.
		inline Long STORM_FN inH() const { return inMin() / 60; }

		// To string.
		void STORM_FN toS(StrBuf *to) const;
	};

	namespace time {
		// Create durations in various units.
		Duration STORM_FN h(Long v);
		Duration STORM_FN min(Long v);
		Duration STORM_FN s(Long v);
		Duration STORM_FN ms(Long v);
		Duration STORM_FN us(Long v);
	}

	// Sleep for the specified duration. Do not expect more than ms precision.
	void STORM_FN sleep(Duration d);

	// Yield the current thread. This causes the current UThread to stop running, and the system
	// will schedule another thread that is ready to run if possible. The current thread is still
	// marked as ready, and will continue to execute whenever the scheduler schedules it again.
	void STORM_FN yield();

	// Output.
	wostream &operator <<(wostream &to, Moment m);
	wostream &operator <<(wostream &to, Duration d);

	// Arithmetic operations between durations and moments.
	inline Duration STORM_FN operator -(Moment a, Moment b) { return Duration(a.v - b.v); }
	inline Moment STORM_FN operator +(Moment a, Duration b) { return Moment(a.v + b.v); }
	inline Moment STORM_FN operator +(Duration b, Moment a) { return Moment(a.v + b.v); }
	inline Moment STORM_FN operator -(Moment a, Duration b) { return Moment(a.v - b.v); }
	inline Moment &STORM_FN operator +=(Moment &a, Duration b) { a.v += b.v; return a; }
	inline Moment &STORM_FN operator -=(Moment &a, Duration b) { a.v -= b.v; return a; }

	inline Duration STORM_FN operator +(Duration a, Duration b) { return Duration(a.v + b.v); }
	inline Duration STORM_FN operator -(Duration a, Duration b) { return Duration(a.v - b.v); }
	inline Duration STORM_FN operator *(Duration a, Int factor) { return Duration(a.v * factor); }
	inline Duration STORM_FN operator /(Duration a, Int factor) { return Duration(a.v / factor); }
	inline Duration STORM_FN operator *(Duration a, Float factor) { return Duration(Long(a.v * Double(factor))); }
	inline Duration STORM_FN operator /(Duration a, Float factor) { return Duration(Long(a.v / Double(factor))); }
	inline Float STORM_FN operator /(Duration a, Duration b) { return float(double(a.v) / double(b.v)); }
	inline Duration STORM_FN operator %(Duration a, Duration b) { return Duration(a.v % b.v); }
	inline Duration &STORM_FN operator +=(Duration &a, Duration b) { a.v += b.v; return a; }
	inline Duration &STORM_FN operator -=(Duration &a, Duration b) { a.v -= b.v; return a; }
	inline Duration &STORM_FN operator *=(Duration &a, Int b) { a.v *= b; return a; }
	inline Duration &STORM_FN operator /=(Duration &a, Int b) { a.v /= b; return a; }
	inline Duration &STORM_FN operator *=(Duration &a, Float b) { a.v = Long(a.v * Double(b)); return a; }
	inline Duration &STORM_FN operator /=(Duration &a, Float b) { a.v = Long(a.v / Double(b)); return a; }

	// Comparisons. TODO: Consider wrapping of Moments.
	inline Bool STORM_FN operator ==(Moment a, Moment b) { return a.v == b.v; }
	inline Bool STORM_FN operator !=(Moment a, Moment b) { return a.v != b.v; }
	inline Bool STORM_FN operator >(Moment a, Moment b) { return a.v > b.v; }
	inline Bool STORM_FN operator <(Moment a, Moment b) { return a.v < b.v; }
	inline Bool STORM_FN operator >=(Moment a, Moment b) { return a.v >= b.v; }
	inline Bool STORM_FN operator <=(Moment a, Moment b) { return a.v <= b.v; }

	inline Bool STORM_FN operator ==(Duration a, Duration b) { return a.v == b.v; }
	inline Bool STORM_FN operator !=(Duration a, Duration b) { return a.v != b.v; }
	inline Bool STORM_FN operator <(Duration a, Duration b) { return a.v < b.v; }
	inline Bool STORM_FN operator >(Duration a, Duration b) { return a.v > b.v; }
	inline Bool STORM_FN operator <=(Duration a, Duration b) { return a.v <= b.v; }
	inline Bool STORM_FN operator >=(Duration a, Duration b) { return a.v >= b.v; }

	// Tell Storm about these, without disturbing the C++ namespace.
	namespace impl {
		inline Duration STORM_FN min(Duration a, Duration b) { return ::min(a, b); }
		inline Duration STORM_FN max(Duration a, Duration b) { return ::max(a, b); }
	}

	/**
	 * Simple way of measuring accumulated time of calls. Results are printed at the end of the
	 * program execution. No effort has been made to make it work through DLL boundaries.
	 *
	 * Usage:
	 * CHECK_TIME(L"<identifier>");
	 *
	 * All timings with the same identifier are grouped together at the end.
	 */
	class CheckTime : NoCopy {
	public:
		CheckTime(const wchar_t *id) : id(id), started() {}

		~CheckTime() {
			save(id, Moment() - started);
		}

	private:
		const wchar_t *id;
		Moment started;

		static void save(const wchar_t *id, const Duration &d);
	};

#define CHECK_TIME(id) CheckTime _x(id);

}
