#pragma once
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Core/Array.h"
#include "Core/EnginePtr.h"
#include "Net.h"

namespace storm {
	STORM_PKG(core.net);

	/**
	 * Abstract representation of an *internet address* (e.g. IPv4 or IPv6). Roughly corresponds to
	 * a `struct sockaddr` in UNIX.
	 *
	 * Port = 0 means 'unspecified port'.
	 *
	 * Sadly, it is not possible to subclass this class from Storm, as low-level memory access is
	 * required to interface with the underlying socket implementation. Because of this, no
	 * constructor is exposed to Storm.
	 */
	class Address : public Object {
		STORM_CLASS;
	public:
		// Create.
		Address(Nat port);

		// Get the port.
		Nat STORM_FN port() const { return myPort; }

		// Create a copy bound to a different port.
		Address *STORM_FN withPort(Nat port) const;

		// Fill a 'sockaddr' with the address in here.
		virtual void fill(sockaddr *fill) const;

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Hash.
		virtual Nat STORM_FN hash() const;

		// Equality.
		virtual Bool STORM_FN operator ==(const Address &other) const;

	private:
		// Port.
		Nat myPort;

		// Explicit padding required since the ABI used by GCC differs from MSVC.
		// TODO: Remove the need for this member!
		Nat pad;
	};

	// Convert a sockaddr to a proper Storm class.
	Address *toStorm(Engine &e, sockaddr *src);

	// Unsafe version that requires explicit checking.
	MAYBE(Address *) toStormUnsafe(Engine &e, sockaddr *src);

	// Parse a string containing an address into an appropriate representation. Does *not* resolve names.
	Address *STORM_FN toAddress(Str *addr);

	// Lookup a name on the network.
	// Note: this function currently blocks *everything* running on the current thread while waiting
	// for the network. It could therefore be useful to dispatch it to another thread.
	Array<Address *> *STORM_FN lookupAddress(Str *addr);


	/**
	 * An IPv4 address.
	 */
	class Inet4Address : public Address {
		STORM_CLASS;
	public:
		// Create from sockaddr. Prefer 'toStorm' above.
		Inet4Address(sockaddr_in *src);

		// Create from numbers. 'addr' is the address encoded as a 32-bit integer, with the high
		// bits being the first digits in the address.
		STORM_CTOR Inet4Address(Nat port, Nat addr);

		// Get the raw address.
		Nat STORM_FN data() const { return myAddr; }

		// Access individual parts by index.
		Byte STORM_FN operator [](Nat id) const;

		// Number of bytes. Allows iterating through the individual parts.
		Nat STORM_FN count() const { return 4; }

		// Fill 'sockaddr'.
		virtual void fill(sockaddr *fill) const;

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Hash.
		virtual Nat STORM_FN hash() const;

		// Equality.
		virtual Bool STORM_FN operator ==(const Address &other) const;

	private:
		// The address.
		Nat myAddr;
	};


	/**
	 * An IPv6 address.
	 */
	class Inet6Address : public Address {
		STORM_CLASS;
	public:
		// Create from sockaddr. Prefer 'toStorm' above.
		Inet6Address(sockaddr_in6 *src);

		// Create from numbers. 'addr' is the address encoded as four 32-bit integers, with the high
		// bits being the first digits in the address.
		STORM_CTOR Inet6Address(Nat port, Nat addr0, Nat addr1, Nat addr2, Nat addr3);
		STORM_CTOR Inet6Address(Nat port, Nat addr0, Nat addr1, Nat addr2, Nat addr3, Nat flow, Nat scope);

		// Access individual parts by index. Each part is 16 bits long.
		Nat STORM_FN operator [](Nat id) const;

		// Number of 16-bit parts. Allows iterating through the individual parts.
		Nat STORM_FN count() const { return 8; }

		// Get flow info.
		Nat STORM_FN flowInfo() const { return myFlow; }

		// Get scope.
		Nat STORM_FN scope() const { return myScope; }

		// Fill 'sockaddr'.
		virtual void fill(sockaddr *fill) const;

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Hash.
		virtual Nat STORM_FN hash() const;

		// Equality.
		virtual Bool STORM_FN operator ==(const Address &other) const;

	private:
		// The address.
		Nat myAddr0;
		Nat myAddr1;
		Nat myAddr2;
		Nat myAddr3;

		// Flow info and scope.
		Nat myFlow;
		Nat myScope;
	};

}
