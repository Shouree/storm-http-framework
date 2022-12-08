#pragma once

namespace storm {

	/**
	 * Install suitable handlers that convert system exceptions (e.g. division by zero) into regular
	 * Storm exceptions.
	 *
	 * Call the function below whenever an Engine is created to set up the conversion. This is done
	 * per-process, so it technically only needs to be called once per process. It is not a problem
	 * if the function is called more than once.
	 */
	void setupSystemExceptions();

}
