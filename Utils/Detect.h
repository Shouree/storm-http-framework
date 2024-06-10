#pragma once

/**
 * Detect members in a class. From: http://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Member_Detector
 *
 * Usage:
 * CREATE_DETECTOR(attribute)
 * or
 * CREATE_DETECTOR_MEMBER(suffix, member)
 * where 'suffix' is used in the name of the detect-class, and "memeber" is checked for.
 *
 * detect_<attribute><T>::value -> true or false.
 */
#define CREATE_DETECTOR_MEMBER(suffix, member)							\
	template <class T>													\
	class detect_##suffix {												\
		struct Fallback { int member; };								\
		struct Derived : T, Fallback {};								\
																		\
		template <class U, U> struct Check;								\
																		\
		typedef char ArrayOfOne[1];										\
		typedef char ArrayOfTwo[2];										\
																		\
		template <class U>												\
		static ArrayOfOne &func(Check<int Fallback::*, &U::member> *);	\
		template <class U>												\
		static ArrayOfTwo &func(...);									\
	public:																\
	typedef detect_##suffix type;										\
	enum { value = sizeof(func<Derived>(0)) == sizeof(ArrayOfTwo) };	\
	};																	\
	template <>															\
	class detect_##suffix<int> {										\
	public:																\
	enum { value = false };												\
	};																	\
	template <>															\
	class detect_##suffix<nat> {										\
	public:																\
	enum { value = false };												\
	};																	\
	template <>															\
	class detect_##suffix<byte> {										\
	public:																\
	enum { value = false };												\
	};																	\
	template <>															\
	class detect_##suffix<char> {										\
	public:																\
	enum { value = false };												\
	};																	\
	template <>															\
	class detect_##suffix<wchar_t> {									\
	public:																\
	enum { value = false };												\
	};																	\
	template <>															\
	class detect_##suffix<bool> {										\
	public:																\
	enum { value = false };												\
	};																	\
	template <>															\
	class detect_##suffix<void> {										\
	public:																\
	enum { value = false };												\
	};

#define CREATE_DETECTOR(X) CREATE_DETECTOR_MEMBER(X, X)
