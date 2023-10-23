#pragma once
#include "Stream.h"
#include "Core/Lock.h"
#include "Core/Event.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * A class that contains two streams, an input and an output, that are connected to each other
	 * in a manner similar to a POSIX pipe.
	 *
	 * The class has a circular buffer of a fixed size (4 KB by default). When a thread attempts to
	 * read from an empty pipe, the thread is blocked. The same happens when a thread attempts to
	 * write to a full pipe.
	 *
	 * The pipe may be used to communicate between different threads, as each end of the pipe remain
	 * pointing to the same logical pipe even after they are copied. This is not the case for the
	 * Pipe class itself, however.
	 */
	class Pipe : public Object {
		STORM_CLASS;
	public:
		// Create a pipe, use the default size of 4 KB.
		STORM_CTOR Pipe();

		// Create a pipe, specify the buffer size (in bytes).
		STORM_CTOR Pipe(Nat size);

		// Get the read end of the pipe.
		IStream *STORM_FN input();

		// Get the write end of the pipe.
		OStream *STORM_FN output();

		// Close the entire pipe.
		void STORM_FN close();

	private:
		// Data.
		GcArray<Byte> *data;

		// Read pointer (next byte to be read).
		Nat readPos;

		// Number of bytes in the circular queue.
		Nat filled;

		// Which ends are closed.
		Bool readClosed;
		Bool writeClosed;

		// Lock for synchronizing access.
		Lock *lock;

		// Event, wait for being able to read.
		Event *waitRead;

		// Event, wait for being able to write.
		Event *waitWrite;

		// Initialize.
		void init(Nat size);

		// Called by the IStream:
		friend class PipeIStream;
		Bool more();
		void read(Buffer to);
		void peek(Buffer to);
		void closeRead();

		// Common code for "read" and "peek".
		void readCommon(Buffer to, Bool updatePos);

		// Called by the OStream:
		friend class PipeOStream;
		void write(Buffer from, Nat start);
		void closeWrite();
	};


	/**
	 * IStream used by the Pipe class.
	 */
	class PipeIStream : public IStream {
		STORM_CLASS;
	public:
		// Create. Not exposed to Storm.
		PipeIStream(Pipe *pipe) : pipe(pipe) {}

		// Close.
		void STORM_FN close() override {
			pipe->closeRead();
		}

		// More data?
		Bool STORM_FN more() override {
			return pipe->more();
		}

		// Read data.
		Buffer STORM_FN read(Buffer to) override {
			pipe->read(to);
			return to;
		}

		// Peek data.
		Buffer STORM_FN peek(Buffer to) override {
			pipe->peek(to);
			return to;
		}

	private:
		// Pipe we're connected to.
		Pipe *pipe;
	};


	/**
	 * OStream used by the Pipe class.
	 */
	class PipeOStream : public OStream {
		STORM_CLASS;
	public:
		// Create. Not exposed to Storm.
		PipeOStream(Pipe *pipe) : pipe(pipe) {}

		// Close.
		void STORM_FN close() override {
			pipe->closeWrite();
		}

		// Write data.
		void STORM_FN write(Buffer buf, Nat start) override {
			pipe->write(buf, start);
		}

		// Flush (no-op for Pipes).
		void STORM_FN flush() override {}

	private:
		// Pipe we're connected to.
		Pipe *pipe;
	};

}
