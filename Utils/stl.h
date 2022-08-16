#pragma once

#include <functional>

class NoCaseLess {
public:
	bool operator ()(const String &lhs, const String &rhs) const;
};

// template <class T>
// class ptr_less : std::binary_function<T, T, bool> {
// 	bool operator() (const T *x, const T *y) { return *x < *y; }
// };
