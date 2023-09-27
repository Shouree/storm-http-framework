#pragma once
#include <vector>

/**
 * Import external packages.
 *
 * Note: "into" might be null. In that case a suitable name should be extracted from
 * "path" (which might be a file).
 */
struct Import {
	const wchar_t *path;
	const wchar_t *into;
	bool tryRun;
};

/**
 * Parameters from the command line.
 */
class Params {
public:
	Params(int argc, const wchar_t *argv[]);

	/**
	 * What mode shall we run in?
	 */
	enum Mode {
		modeHelp,
		modeAuto,
		modeRepl,
		modeFunction,
		modeTests,
		modeTestsRec,
		modeServer,
		modeVersion,
		modeError,
	};

	// Mode.
	Mode mode;

	// Parameter to the mode (might be null).
	const wchar_t *modeParam;

	// Second parameter to the mode (might be null).
	const wchar_t *modeParam2;

	// Root path (null if default).
	const wchar_t *root;

	// Import additional packages.
	vector<Import> import;

	// Parameters to the program.
	vector<const wchar_t *> argv;
};

void help(const wchar_t *cmd);
