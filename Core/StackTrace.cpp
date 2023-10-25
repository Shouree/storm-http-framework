#include "stdafx.h"
#include "StackTrace.h"
#include "StrBuf.h"
#include "Convert.h"
#include "Utils/StackInfoSet.h"

namespace storm {

	StackTrace::StackTrace(Engine &e) : frames(new (e) Array<Frame>()) {}

	StackTrace::Frame::Frame(void *base, Nat offset, Nat id) : base(base), offset(offset), id(id) {}

	void StackTrace::deepCopy(CloneEnv *env) {
		frames = new (env) Array<Frame>(*frames);
		// No need to call 'deepCopy'.
	}

	class StrBufOut : public GenericOutput {
	public:
		StrBufOut(StrBuf *to) : to(to), frameNumber(0) {}

		virtual void put(const wchar *str) { *to << str; }
		virtual void put(const char *str) { *to << toWChar(to->engine(), str)->v; }
		virtual void put(size_t i) { *to << i; }
		virtual void putHex(size_t i) { *to << hex(i); }

		virtual void nextFrame() {
			if (frameNumber > 0)
				*to << S("\n");
			*to << width(3) << (frameNumber++) << S(": ");
		}
	private:
		StrBuf *to;
		Nat frameNumber;
	};

	void StackTrace::format(StrBuf *to) const {
		StackInfoSet &s = stackInfo();
		StrBufOut adapter(to);

		for (Nat i = 0; i < frames->count(); i++) {
			adapter.nextFrame();

			Frame f = frames->at(i);
			s.format(adapter, f.id, f.base, f.offset);
		}
	}

	Str *StackTrace::format() const {
		StrBuf *out = new (frames) StrBuf();
		format(out);
		return out->toS();
	}

	wostream &operator <<(wostream &to, const StackTrace &trace) {
		for (Nat i = 0; i < trace.count(); i++) {
			if (i > 0)
				to << std::endl;
			to << L"Frame " << std::setw(3) << i << L": at 0x" << trace[i].ptr();
		}
		return to;
	}

	StrBuf &operator <<(StrBuf &to, StackTrace trace) {
		for (Nat i = 0; i < trace.count(); i++) {
			if (i > 0)
				to << S("\n");
			to << width(3) << i << S(": at 0x") << hex(trace[i].ptr());
		}
		return to;
	}

	class StormGen : public TraceGen {
	public:
		StackTrace &result;

		StormGen(StackTrace &result) : result(result) {}

		void init(size_t count) {
			if (count > 0)
				result.reserve(Nat(count));
		}

		void put(const StackFrame &frame) {
			result.push(StackTrace::Frame(frame.fnBase, frame.offset, frame.id));
		}
	};

	StackTrace collectStackTrace(EnginePtr e) {
		return collectStackTrace(e, (void *)null);
	}

	StackTrace collectStackTrace(EnginePtr e, Nat ignore) {
		StackTrace result(e.v);
		StormGen gen(result);

		createStackTrace(gen, ignore, null);

		return result;
	}

	StackTrace collectStackTrace(EnginePtr e, void *state) {
		StackTrace result(e.v);
		StormGen gen(result);

		createStackTrace(gen, 0, state);

		return result;
	}

}
