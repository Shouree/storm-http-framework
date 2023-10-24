#include "stdafx.h"
#include "Open.h"
#include "Error.h"
#include "Core/Str.h"
#include "Core/Convert.h"

#if defined(WINDOWS)

#include <shellapi.h>
#pragma comment (lib, "Shell32.lib")

namespace storm {

	void open(Url *file) {
		DWORD error = (DWORD)ShellExecute(NULL, L"open", file->toS()->c_str(), NULL, NULL, SW_SHOWDEFAULT);
		const wchar *msg = null;
		switch (error) {
		case 0:
		case SE_ERR_OOM:
			msg = S("Out of memory.");
			break;
		case ERROR_FILE_NOT_FOUND:
			msg = S("File not found.");
			break;
		case ERROR_PATH_NOT_FOUND:
			msg = S("Path not found.");
			break;
		case SE_ERR_ACCESSDENIED:
			msg = S("Access denied.");
			break;
		case SE_ERR_ASSOCINCOMPLETE:
		case SE_ERR_NOASSOC:
			msg = S("Unknown or incomplete file association.");
			break;
		default:
			if (error <= 32)
				msg = S("Unknown error.");
		}

		if (msg)
			throw new (file) SystemError(TO_S(file, S("Failed to open ") << file << S(": ") << msg));
	}

	// Escape the string so that it can be read back by most C programs on Windows. Adds quotes around it.
	static void escapeStr(StrBuf *out, Str *input) {
		*out << Char('"');

		Nat slashes = 0;
		for (Str::Iter i = input->begin(); i != input->end(); ++i) {
			if (i.v().codepoint() == '\\') {
				slashes++;
			} else if (i.v().codepoint() == '"') {
				// If we have slashes right before the ", then we need to double those since they
				// are interpreted as an escape character in this particular context.
				for (Nat i = 0; i < slashes; i++)
					*out << Char('\\');
				*out << Char('"');
				slashes = 0;
			} else {
				slashes = 0;
			}

			*out << i.v();
		}

		// If we have slashes here, we need to double them once more.
		for (Nat i = 0; i < slashes; i++)
			*out << Char('\\');
		*out << Char('"');
	}

	// Convert parameters to a string.
	static Str *strParams(Str *program, Array<Str *> *params) {
		StrBuf *out = new (program) StrBuf();
		escapeStr(out, program);

		for (Nat i = 0; i < params->count(); i++) {
			*out << Char(' ');
			escapeStr(out, params->at(i));
		}

		return out->toS();
	}

	void execute(Url *file, Array<Str *> *params) {
		execute(file, params, null);
	}

	void execute(Url *file, Array<Str *> *params, MAYBE(Url *) workingDirectory) {
		STARTUPINFO info;
		zeroMem(info);
		info.cb = sizeof(info);

		PROCESS_INFORMATION out;

		Str *fileName;
		if (file->relative() && file->count() == 1) {
			fileName = file->at(0);
		} else {
			fileName = file->toS();
		}

		const wchar *cwd = null;
		if (workingDirectory)
			cwd = workingDirectory->format()->c_str();

		const wchar *p = strParams(fileName, params)->c_str();
		if (!CreateProcess(fileName->c_str(), (LPWSTR)p, NULL, NULL, FALSE, 0, NULL, cwd, &info, &out)) {
			const wchar *error = S("Unknown error.");
			switch (GetLastError()) {
			case ERROR_FILE_NOT_FOUND:
				error = S("File not found.");
				break;
			case ERROR_PATH_NOT_FOUND:
				error = S("Path not found.");
				break;
			case ERROR_ACCESS_DENIED:
				error = S("Access denied.");
				break;
			}

			throw new (file) SystemError(TO_S(file, S("Failed to launch ") << file << S(": ") << error));
		}

		CloseHandle(out.hProcess);
		CloseHandle(out.hThread);
	}

}

#elif defined(POSIX)

#include <unistd.h>
#include <sys/wait.h>
#include <sys/fcntl.h>

namespace storm {

	// Spawn a new process. Properly indicates failures from "execlp" to the parent process.
	// 'params' is an array of parameters as one would give the execvp() function.
	// Makes sure we don't have to wait for the child process.
	// If "monitorExit" is nonzero, then we wait (maximum of that many seconds) until the process was terminated and
	// return the exit code. Returns - SIGALRM if the timeout expired.
	static int execute(Engine &e, const char *params[], const char *cdTo, nat monitorExit = 0) {
		// To detect errors in the child, we create a new pipe with the CLOEXEC flag set. As such,
		// we can wait until the write end is closed. If nothing was written, then we know that
		// everything went well (at least up to starting the process). Otherwise we write an error
		// message to the pipe before exiting.
		int pipefds[2];
		if (pipe(pipefds) != 0)
			throw new (e) SystemError(S("Failed to create a pipe for spawning a process."));

		int pipeRead = pipefds[0];
		int pipeWrite = pipefds[1];
		fcntl(pipeWrite, F_SETFD, FD_CLOEXEC);

		pid_t pid = fork();
		if (pid < 0) {
			close(pipeRead);
			close(pipeWrite);
			throw new (e) SystemError(S("Failed to spawn a child process."));
		}

		if (pid == 0) {
			// Child.
			// Close the read end of the pipe.
			close(pipeRead);

			// Note: We need to use _exit here, otherwise we might crash in an exit cleanup handler somewhere...

			// CD to the correct directory if we were asked to do so.
			if (cdTo) {
				if (chdir(cdTo) < 0) {
					const char *msg = "Failed to change working directory to: ";
					(void)!write(pipeWrite, msg, strlen(msg));
					(void)!write(pipeWrite, cdTo, strlen(cdTo));
					_exit(1);
				}
			}

			// Fork once more to avoid zombies.
			pid = fork();
			if (pid < 0) {
				const char *msg = "Failed to spawn a child process.";
				(void)!write(pipeWrite, msg, strlen(msg));
				_exit(1);
			}

			if (pid == 0) {
				// Child. Now we can call exec!
				execvp(params[0], (char **)params);

				// If we get here, the call failed.
				const char *prefix = "Failed to execute the child: ";
				const char *error = strerror(errno);
				(void)!write(pipeWrite, prefix, strlen(prefix));
				(void)!write(pipeWrite, error, strlen(error));
				_exit(1);
			}

			// Parent. See if we should wait for the exit code from the child.
			if (monitorExit) {
				alarm(monitorExit);

				int status = 0;
				waitpid(pid, &status, 0);

				// Forward it.
				if (WIFEXITED(status))
					_exit(WEXITSTATUS(status));
				if (WIFSIGNALED(status))
					raise(WTERMSIG(status));
			}

			_exit(100);
		}

		// Close the write end from here.
		close(pipeWrite);

		// Wait for the first child.
		int status = 0;
		waitpid(pid, &status, 0);

		// Read from the pipe until we are done.
		char buffer[128];
		StrBuf *message = new (e) StrBuf();
		while (true) {
			ssize_t result = read(pipeRead, buffer, sizeof(buffer));
			if (result < 0) {
				if (errno == EINTR)
					continue;

				perror("Error reading the pipe");
				return 100;
			} else if (result == 0) {
				// No more data. The socket is closed!
				break;
			} else {
				// Save the message.
				*message << toWChar(e, buffer, size_t(result))->v;
			}
		}

		if (message->any())
			throw new (e) SystemError(message->toS());

		if (WIFEXITED(status))
			return WEXITSTATUS(status);
		if (WIFSIGNALED(status))
			return -WTERMSIG(status);
		return 0;
	}

	void open(Url *file) {
		// Note: We are not using "format" here since we want to support HTTP and HTTPS for example.
		const char *params[] = {
			"xdg-open",
			file->toS()->utf8_str(),
			NULL
		};
		// Note: xdg-open does not always terminate immediately, so we cannot really look at its exit code.
		int status = execute(file->engine(), params, null, 1);
		if (status == 2)
			throw new (file) SystemError(TO_S(file, S("The file ") << file << S(" does not exist.")));
		if (status == 3)
			throw new (file) SystemError(TO_S(file, S("Unable to find a tool for opening ") << file));
		if (status != 0 && status != -SIGALRM)
			throw new (file) SystemError(TO_S(file, S("Failed to open: ") << file));
	}

	void execute(Url *file, Array<Str *> *params) {
		execute(file, params, null);
	}

	void execute(Url *file, Array<Str *> *params, Url *workingDir) {
		const char **cParam = new const char *[params->count() + 2];
		try {
			// If "file" is relative and only has a single part, then don't output "./" in front of
			// it. That would probably inhibit the ability to look in PATH.
			if (file->relative() && file->count() == 1) {
				cParam[0] = file->at(0)->utf8_str();
			} else {
				cParam[0] = file->format()->utf8_str();
			}

			for (Nat i = 0; i < params->count(); i++)
				cParam[i + 1] = params->at(i)->utf8_str();
			cParam[params->count() + 1] = NULL;

			const char *cwd = null;
			if (workingDir)
				cwd = workingDir->format()->utf8_str();

			execute(file->engine(), cParam, cwd);

			delete []cParam;
		} catch (...) {
			delete []cParam;
			throw;
		}
	}
}

#endif

