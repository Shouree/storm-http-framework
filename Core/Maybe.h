#pragma once

namespace storm {

	/**
	 * Custom Maybe-type for interoperability with C++. It is only intended for value types, and
	 * aimed at compatibility between C++ and Storm as parameters and return values. As such, a
	 * number of limitations apply. The preprocessor will warn about most of these.
	 * - This class can *only* be used with value types. Use MAYBE(X *) for class-types.
	 * - Classes that rely on custom constructors/destructors are not properly handled
	 *   (they will be created/destroyed even if the data is not present).
	 * - Classes without a default constructor are not supported (for the above reason).
	 */
	template <class T>
	class Maybe {
	public:
		// Create an empty value.
		Maybe() : data(), present(false) {}

		// Create a value.
		explicit Maybe(const T &data) : data(data), present(true) {}

		// Any value?
		inline Bool any() const {
			return present;
		}
		inline Bool empty() const {
			return !present;
		}

		// Get the value. Will not check whether or not the value is present.
		T &value() { return data; }
		const T &value() const { return data; }

	private:
		// The actual data.
		T data;

		// If there is a value present or not.
		Bool present;
	};

}
