#include "stdafx.h"
#include "Timing.h"
#include "Str.h"
#include "StrBuf.h"
#include <iomanip>

namespace storm {

#if defined(WINDOWS)

	static bool initialized = false;
	static Long resolution;

	static void initialize() {
		LARGE_INTEGER r;
		QueryPerformanceFrequency(&r);
		resolution = r.QuadPart;
	}

	static Long now() {
		if (!initialized)
			initialize();

		LARGE_INTEGER value;
		QueryPerformanceCounter(&value);
		Long v = value.QuadPart;

		// Avoid overflows when 'v' is large. Otherwise we may multiply 'v' with
		// 1000000 and overflow before dividing the number, which effectively reduces
		// the valid range of our return value. This solution reduces that issue.
		Long seconds = v / resolution;
		Long remaining = v % resolution;
		const Long s = 1000 * 1000;

		return seconds * s + (remaining * s) / resolution;
	}

#elif defined(POSIX)

	static Long now() {
		struct timespec time = {0, 0};
		clock_gettime(CLOCK_MONOTONIC, &time);

		Long r = time.tv_sec;
		r *= 1000 * 1000;
		r += time.tv_nsec / 1000;

		return r;
	}

#else
#error "Please implement some time-keeping for your platform."
#endif

	Moment::Moment() : v(now()) {}

	Moment::Moment(Long v) : v(v) {}

	struct Unit {
		const wchar_t *name;
		Long factor;
	};

	static const Unit units[] = {
		{ L"us", 1 },
		{ L"ms", 1000 },
		{ L"s", 1000 },
		{ L"min", 60 },
		{ L"h", 60 },
		{ L"d", 24 },
	};

	wostream &operator <<(wostream &to, Moment m) {
		return to << L"@" << m.v << L" us";
	}

	wostream &operator <<(wostream &to, Duration d) {
		Long t = abs(d.v);

		Long div = units[0].factor;
		const wchar_t *unit = units[0].name;

		for (nat i = 1; i < ARRAY_COUNT(units); i++) {
			if ((t / div) < units[i].factor)
				break;

			div *= units[i].factor;
			unit = units[i].name;
		}

		return to << std::fixed << std::setprecision(2) << (d.v / double(div)) << L" " << unit;
	}

	void Moment::toS(StrBuf *to) const {
		*to << L"@" << v << L" us";
	}

	void Duration::toS(StrBuf *to) const {
		Long t = abs(v);

		Long div = units[0].factor;
		const wchar_t *unit = units[0].name;

		for (Nat i = 1; i < ARRAY_COUNT(units); i++) {
			if ((t / div) < units[i].factor)
				break;

			div *= units[i].factor;
			unit = units[i].name;
		}

		SaveFormat save(to);
		*to << fixed(2) << (v / double(div)) << L" " << unit;
	}

	Duration::Duration() : v(0) {}

	Duration::Duration(Long v) : v(v) {}

	namespace time {

		Duration h(Long v) {
			return Duration(Long(v) * Long(60) * Long(60) * Long(1000) * Long(1000));
		}

		Duration min(Long v) {
			return Duration(Long(v) * Long(60) * Long(1000) * Long(1000));
		}

		Duration s(Long v) {
			return Duration(Long(v) * Long(1000) * Long(1000));
		}

		Duration ms(Long v) {
			return Duration(Long(v) * 1000);
		}

		Duration us(Long v) {
			return Duration(v);
		}

	}

	void sleep(Duration d) {
		os::UThread::sleep(nat(d.inMs()));
	}

	void yield() {
		os::UThread::leave();
	}


	/**
	 * Timing.
	 */

	class TimeKeeper : NoCopy {
	public:
		typedef map<String, Duration> Data;

		~TimeKeeper() {
			if (data.size() > 0) {
				PLN(L"\nMeasured run-times:\n");
			}

			for (Data::iterator i = data.begin(); i != data.end(); ++i) {
				PLN(std::setw(10) << i->first << L": " << i->second);
			}
		}

		void save(const String &id, const Duration &d) {
			data[id] += d;
		}

	private:
		// All times currently known.
		Data data;
	};

	static TimeKeeper &keeper() {
		static TimeKeeper k;
		return k;
	}

	void CheckTime::save(const wchar_t *id, const Duration &d) {
		keeper().save(id, d);
	}

}
