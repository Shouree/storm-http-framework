#pragma once

namespace sql {

	/**
	 * String utilities.
	 */

	// Compare strings in a case-insensitive manner.
	// For simplicity, it is assumed that the "to" consists of ASCII only.
	bool compareNoCase(Str *compare, const wchar *to);
	bool compareNoCase(const wchar *compareBegin, const wchar *compareEnd, const wchar *to);

	// Embed a string as a SQL literal.
	Str *STORM_FN toSQLStrLiteral(Str *x);

}
