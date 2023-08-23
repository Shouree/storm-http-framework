#include "stdafx.h"
#include "ConnectionType.h"

namespace sql {

	ConnectionType::ConnectionType() : data(null) {}

	ConnectionType ConnectionType::socket(Address *address) {
		ConnectionType c;
		c.data = address;
		return c;
	}

	ConnectionType ConnectionType::local(Str *name) {
		ConnectionType c;
		c.data = name;
		return c;
	}

	ConnectionType ConnectionType::local() {
		return ConnectionType();
	}

	MAYBE(Address *) ConnectionType::isSocket() const {
		return as<Address>(data);
	}

	MAYBE(Str *) ConnectionType::isLocal() const {
		return as<Str>(data);
	}

	StrBuf *operator <<(StrBuf *to, const ConnectionType &c) {
		if (Address *a = c.isSocket())
			*to << S("socket: ") << a;
		else if (Str *l = c.isLocal())
			*to << S("local socket/pipe: ") << l;
		else
			*to << S("local, default");
		return to;
	}


}
