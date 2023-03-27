#pragma once

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * How far in the boot process are we? Ie. how much is currently working?
	 */
	enum BootStatus {
		// Assume nothing is working.
		bootNone,

		// Regular types can be created.
		bootTypes,

		// Template types can be created.
		bootTemplates,

		// Packages are working and populated.
		bootPackages,

		// ...

		// Everything is working.
		bootDone,

		// Finalizing in progress.
		bootShutdown,

		// No 'world' anymore.
		bootLateShutdown,
	};

}
