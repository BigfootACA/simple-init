#ifndef UTIL_LINUX_CLOSESTREAM_H
#define UTIL_LINUX_CLOSESTREAM_H

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <unistd.h>
#include "defines.h"

#ifndef CLOSE_EXIT_CODE
# define CLOSE_EXIT_CODE EXIT_FAILURE
#endif

static inline int
close_stream(FILE * stream)
{
	const int some_pending = (__fpending(stream) != 0);
	const int prev_fail = (ferror(stream) != 0);
	const int fclose_fail = (fclose(stream) != 0);

	if (prev_fail || (fclose_fail && (
					  some_pending ||
					  errno != EBADF))) {
		if (!fclose_fail && !(errno == EPIPE))
			errno = 0;
		return EOF;
	}
	return 0;
}

static inline int
flush_standard_stream(FILE *stream)
{
	int fd;

	errno = 0;

	if (ferror(stream) != 0 || fflush(stream) != 0)
		goto error;

	/*
	 * Calling fflush is not sufficient on some filesystems
	 * like e.g. NFS, which may defer the actual flush until
	 * close. Calling fsync would help solve this, but would
	 * probably result in a performance hit. Thus, we work
	 * around this issue by calling close on a dup'd file
	 * descriptor from the stream.
	 */
	if ((fd = fileno(stream)) < 0 || (fd = dup(fd)) < 0 || close(fd) != 0)
		goto error;

	return 0;
error:
	return (errno == EBADF) ? 0 : EOF;
}

/* Meant to be used atexit(close_stdout); */
static inline void
close_stdout(void)
{
	if (flush_standard_stream(stdout) != 0 && !(errno == EPIPE)) {
		if (errno)
			warn(_("write error"));
		else
			warnx(_("write error"));
		_exit(CLOSE_EXIT_CODE);
	}

	if (flush_standard_stream(stderr) != 0)
		_exit(CLOSE_EXIT_CODE);
}

static inline void
close_stdout_atexit(void)
{
	/*
	 * Note that close stdout at exit disables ASAN to report memory leaks
	 */
	atexit(close_stdout);
}

static inline int
close_fd(int fd)
{
	const int fsync_fail = (fsync(fd) != 0);
	const int close_fail = (close(fd) != 0);

	if (fsync_fail || close_fail)
		return EOF;
	return 0;
}

#endif /* UTIL_LINUX_CLOSESTREAM_H */
