#include "stdafx.h"
#include "Host.h"

namespace sql {

	Host::Host() : data(null) {}

	Host Host::socket(Address *address) {
		Host c;
		c.data = address;
		return c;
	}

	Host Host::local(Str *name) {
		Host c;
		c.data = name;
		return c;
	}

	Host Host::local() {
		return Host();
	}

	MAYBE(Address *) Host::isSocket() const {
		return as<Address>(data);
	}

	MAYBE(Str *) Host::isLocal() const {
		return as<Str>(data);
	}

	void Host::toS(StrBuf *to) const {
		if (Address *a = isSocket())
			*to << S("socket: ") << a;
		else if (Str *l = isLocal())
			*to << S("local socket/pipe: ") << l;
		else
			*to << S("local, default");
	}


}
