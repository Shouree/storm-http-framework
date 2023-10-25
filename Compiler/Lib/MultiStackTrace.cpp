#include "stdafx.h"
#include "MultiStackTrace.h"
#include "Exception.h"
#include "Thread.h"
#include "OS/Thread.h"
#include "OS/UThread.h"
#include "Compiler/Engine.h"
#include "Compiler/Package.h"

namespace storm {

	static const Nat framesToSkip = 5;

	ThreadStackTraces::ThreadStackTraces(Nat count) {
		data = runtime::allocArray<StackTrace>(engine(), StormInfo<StackTrace>::handle(engine()).gcArrayType, count);
		data->filled = 0;
	}

	Nat ThreadStackTraces::count() const {
		return Nat(data->filled);
	}

	StackTrace ThreadStackTraces::operator [](Nat elem) const {
		if (elem >= data->filled)
			throw new (this) ArrayError(elem, Nat(data->filled));
		else
			return data->v[elem];
	}

	Nat ThreadStackTraces::push(StackTrace trace) {
		if (data->filled >= data->count)
			return Nat(data->count);

		Nat id = Nat(data->filled++);
		new (Place(&data->v[id])) StackTrace(trace);
		return id;
	}

	void ThreadStackTraces::toS(StrBuf *to) const {
		*to << S("Thread ");
		if (threadName) {
			*to << threadName << S(" (") << hex((void *)threadId) << S(")");
		} else {
			*to << hex((void *)threadId);
		}
		*to << S(":\n");

		Indent z(to);
		for (Nat i = 0; i < data->filled; i++) {
			if (i > 0)
				*to << S("\n");
			*to << S("UThread ") << i << S(":\n");
			data->v[i].format(to);
		}
	}


	MultiThreadStackTraces::MultiThreadStackTraces(Nat count) {
		data = runtime::allocArray<ThreadStackTraces *>(engine(), &pointerArrayType, count);
		data->filled = data->count;
	}

	Nat MultiThreadStackTraces::count() const {
		return Nat(data->filled);
	}

	ThreadStackTraces *MultiThreadStackTraces::operator [](Nat elem) const {
		if (elem >= data->filled)
			throw new (this) ArrayError(elem, Nat(data->filled));
		else
			return data->v[elem];
	}

	void MultiThreadStackTraces::set(Nat id, ThreadStackTraces *elem) {
		if (id < data->count)
			data->v[id] = elem;
	}

	void MultiThreadStackTraces::toS(StrBuf *to) const {
		for (Nat i = 0; i < data->count; i++) {
			if (i > 0)
				*to << S("\n");

			ThreadStackTraces *t = data->v[i];
			if (!t)
				continue;

			t->toS(to);
		}
	}



	// Global trace data. Shared between threads. Assumed to be stack allocated.
	struct SharedTrace {
		// Engine.
		Engine &e;

		// Semaphore used to signal from the threads.
		os::Sema signal;

		// Semaphore used by the threads to wait for an event.
		Semaphore wait;

		// The main thread.
		os::Thread main;

		// The set of stack traces that we are creating.
		MultiThreadStackTraces *output;

		// Create.
		SharedTrace(Engine &e, Nat threads) : e(e), signal(0), wait(0), main(os::Thread::current()) {
			output = new (e) MultiThreadStackTraces(threads);
		}
	};

	// An instance of this class is created for each thread we create stack traces for. It keeps
	// track of the state in each thread and contains functions executed at various stages of the
	// capture.
	struct ThreadTrace {
		// Shared data.
		SharedTrace &shared;

		// The ID into 'output' in 'shared' that we use to store our output. We can't store the
		// pointer here, since this object is allocated on the regular C++ heap.
		Nat traceId;

		// Create.
		ThreadTrace(SharedTrace &shared) : shared(shared) {}

		// Main function for the trace.
		void main() {
			prepare();

			// Wait for our signal!
			shared.wait.down();

			capture(false);
		}

		// Preparations.
		void prepare() {
			// Make it known that we're alive.
			shared.signal.up();
		}

		// Capture.
		void capture(bool thisThread) {
			os::UThreadState *current = os::UThreadState::current();
			vector<os::UThread> stacks = current->idleThreads();

			Nat threadCount = Nat(stacks.size());
			if (thisThread)
				threadCount++;

			ThreadStackTraces *traces = new (shared.e) ThreadStackTraces(threadCount);
			traces->threadId = os::Thread::current().id();
			shared.output->set(traceId, traces);

			if (thisThread)
				traces->push(collectStackTrace(shared.e, framesToSkip));

			for (size_t i = 0; i < stacks.size(); i++) {
				if (!stacks[i].detour(util::memberVoidFn(this, &ThreadTrace::captureUThread))) {
					WARNING(L"Failed to execute detour!");
				}
			}

			// Signal again!
			shared.signal.up();
		}

		// Function called from all UThreads in the context of that UThread. Performs the actual
		// capture of the stack trace.
		void captureUThread() {
			ThreadStackTraces *traces = (*shared.output)[traceId];
			traces->push(collectStackTrace(shared.e, framesToSkip));
		}
	};

	static void findThreadsRec(Named *current, MultiThreadStackTraces *traces) {
		if (NamedThread *t = as<NamedThread>(current)) {

			// See if we captured the thread! This does not need to be very fast.
			for (Nat i = 0; i < traces->count(); i++) {
				ThreadStackTraces *trace = (*traces)[i];

				if (t->thread()->sameAs(size_t(trace->threadId))) {
					trace->threadName = t->identifier();
					trace->thread = t->thread();
					return;
				}
			}

			return;
		}

		if (NameSet *search = as<NameSet>(current)) {
			for (NameSet::Iter i = search->begin(), e = search->end(); i != e; ++i) {
				findThreadsRec(i.v(), traces);
			}
		}
	}

	static void findThreads(Engine *e, MultiThreadStackTraces *traces) {
		findThreadsRec(e->package(), traces);
	}

	static void lookupThreadNames(Engine &e, MultiThreadStackTraces *traces) {
		const os::Thread &compiler = Compiler::thread(e)->thread();
		if (compiler != os::Thread::current()) {
			Engine *engine = &e;
			os::Future<void> f;
			os::FnCall<void, 3> p = os::fnCall().add(engine).add(traces);
			os::UThread::spawn(address(&findThreads), true, p, f, &compiler);
			f.result();
		} else {
			findThreads(&e, traces);
		}
	}

	static MultiThreadStackTraces *collectTraces(Engine &e, const vector<os::Thread> &threads) {
		SharedTrace shared(e, Nat(threads.size()));
		vector<ThreadTrace> data(threads.size(), ThreadTrace(shared));
		for (size_t i = 0; i < threads.size(); i++)
			data[i].traceId = Nat(i);

		size_t thisThread = threads.size();

		// Inject a UThread into each thread we are interested in.
		for (size_t i = 0; i < threads.size(); i++) {
			if (threads[i] == shared.main) {
				data[i].prepare();
				thisThread = i;
			} else {
				os::UThread::spawn(util::memberVoidFn(&data[i], &ThreadTrace::main), &threads[i]);
			}
		}

		// Wait for all of them to become ready.
		for (size_t i = 0; i < threads.size(); i++) {
			shared.signal.down();
		}

		// Start the capture!
		for (size_t i = 0; i < threads.size(); i++) {
			shared.wait.up();
		}

		// If we're one of the threads, we also need to do some work...
		if (thisThread < threads.size()) {
			data[thisThread].capture(true);
		}

		// Wait for all of them to complete.
		for (size_t i = 0; i < threads.size(); i++) {
			shared.signal.down();
		}

		// Lookup thread names.
		lookupThreadNames(e, shared.output);

		// Done!
		return shared.output;
	}

	ThreadStackTraces *collectThreadStackTraces(EnginePtr e) {
		MultiThreadStackTraces *result = collectTraces(e.v, vector<os::Thread>(1, os::Thread::current()));
		return (*result)[0];
	}

	MultiThreadStackTraces *collectThreadStackTraces(EnginePtr e, Array<Thread *> *threads) {
		vector<os::Thread> t;
		t.reserve(threads->count());

		for (Nat i = 0; i < threads->count(); i++) {
			t.push_back(threads->at(i)->thread());
		}

		MultiThreadStackTraces *result = collectTraces(e.v, t);

		for (Nat i = 0; i < threads->count(); i++) {
			(*result)[i]->thread = threads->at(i);
		}

		return result;
	}

	MultiThreadStackTraces *collectAllStackTraces(EnginePtr e) {
		return collectTraces(e.v, e.v.allThreads());
	}

}
