#pragma once
#include <vector>

// A container suitable for storage of a set of named file types.
// Mainly intended for use with the pickFile in the UI parts.

class FileTypes {
public:
	// Create with a collection name, such as "Music files", "Video files", or "All supported files"
	FileTypes(const String &collName);

	// Add a new file type. Example: "*.txt", "Text files", or "*.jpg;*.jpeg", "JPEG files"
	void add(const String &ext, const String &name);

	// Add a type supporting all file types.
	void addAll();

	// Get the number of elements.
	inline nat size() const { return (nat)types.size(); }

	struct Elem {
		String ext, name;
	};

	// Get the specified element.
	const Elem &operator[] (nat id) const;

	// Get the collective name of the file types.
	inline const String &name() const { return collName; }

	// Get all types.
	vector<String> allTypes() const;
private:
	String collName;
	vector<Elem> types;
};
