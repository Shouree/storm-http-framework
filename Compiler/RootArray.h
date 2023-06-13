#pragma once
#include "Gc/Gc.h"

namespace storm {

	/**
	 * An array that makes sure to register its contents as a root with the GC.
	 */
	template <class T, bool ambiguous = false>
	class RootArray : NoCopy {
	public:
		// Create.
		RootArray(Gc &gc) : gc(gc), root(null), size(0), data(null) {}

		// Destroy.
		~RootArray() {
			clear();
		}

		// Clear.
		void clear() {
			Gc::destroyRoot(root);
			root = null;
			delete []data;
			data = null;
			size = 0;
		}

		// Resize.
		void resize(nat nSize) {
			if (nSize == 0) {
				clear();
				return;
			}

			if (nSize == size)
				return;

			T **nData = new T*[nSize];
			memset(nData, 0, sizeof(T*)*nSize);
			Gc::Root *nRoot = gc.createRoot(nData, nSize, ambiguous);

			if (data) {
				for (nat i = 0; i < min(nSize, size); i++)
					nData[i] = data[i];

				gc.destroyRoot(root);
				delete []data;
			}

			data = nData;
			size = nSize;
			root = nRoot;
		}

		// Access element.
		T *&operator [](nat id) {
			return data[id];
		}

		T *operator [](nat id) const {
			return data[id];
		}

		// Count.
		nat count() const {
			return size;
		}

	private:
		// Gc used.
		Gc &gc;

		// Current root.
		Gc::Root *root;

		// Array size.
		nat size;

		// Data.
		T **data;
	};

}
