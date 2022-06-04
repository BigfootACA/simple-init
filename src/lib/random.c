/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * General purpose random utilities. Based on libuuid code.
 *
 * This code is free software; you can redistribute it and/or modify it under
 * the terms of the Modified BSD License. The complete text of the license is
 * available in the Documentation/licenses/COPYING.BSD-3-Clause file.
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/random.h>
#include "pathnames.h"

int rand_get_number(int low_n, int high_n)
{
	return rand() % (high_n - low_n + 1) + low_n;
}

static void crank_random(void)
{
	int i;
	struct timeval tv;
	unsigned int n_pid, n_uid;

	gettimeofday(&tv, NULL);
	n_pid = getpid();
	n_uid = getuid();
	srand((n_pid << 16) ^ n_uid ^ tv.tv_sec ^ tv.tv_usec);

	/* Crank the random number generator a few times */
	gettimeofday(&tv, NULL);
	for (i = (tv.tv_sec ^ tv.tv_usec) & 0x1F; i > 0; i--)
		rand();
}

int random_get_fd(void)
{
	int i, fd;

	fd = open(_PATH_DEV_URANDOM, O_RDONLY | O_CLOEXEC);
	if (fd == -1)
		fd = open(_PATH_DEV_RANDOM, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (fd >= 0) {
		i = fcntl(fd, F_GETFD);
		if (i >= 0)
			fcntl(fd, F_SETFD, i | FD_CLOEXEC);
	}
	crank_random();
	return fd;
}

/*
 * Generate a stream of random nbytes into buf.
 * Use /dev/urandom if possible, and if not,
 * use glibc pseudo-random functions.
 */
#define UL_RAND_READ_ATTEMPTS	8
#define UL_RAND_READ_DELAY	125000	/* microseconds */

/*
 * Write @nbytes random bytes into @buf.
 *
 * Returns 0 for good quality of random bytes or 1 for weak quality.
 */
int random_get_bytes(void *buf, size_t nbytes)
{
	unsigned char *cp = (unsigned char *)buf;
	size_t i, n = nbytes;
	int lose_counter = 0;

	while (n > 0) {
		int x;

		errno = 0;
		x = getrandom(cp, n, GRND_NONBLOCK);
		if (x > 0) {			/* success */
		       n -= x;
		       cp += x;
		       lose_counter = 0;
		       errno = 0;
		} else if (errno == ENOSYS) {	/* kernel without getrandom() */
			break;

		} else if (errno == EAGAIN && lose_counter < UL_RAND_READ_ATTEMPTS) {
			usleep(UL_RAND_READ_DELAY);	/* no entropy, wait and try again */
			lose_counter++;
		} else
			break;
	}

	if (errno == ENOSYS)
	/*
	 * We've been built against headers that support getrandom, but the
	 * running kernel does not.  Fallback to reading from /dev/{u,}random
	 * as before
	 */
	{
		int fd = random_get_fd();

		lose_counter = 0;
		if (fd >= 0) {
			while (n > 0) {
				ssize_t x = read(fd, cp, n);
				if (x <= 0) {
					if (lose_counter++ > UL_RAND_READ_ATTEMPTS)
						break;
					usleep(UL_RAND_READ_DELAY);
					continue;
				}
				n -= x;
				cp += x;
				lose_counter = 0;
			}

			close(fd);
		}
	}
	/*
	 * We do this all the time, but this is the only source of
	 * randomness if /dev/random/urandom is out to lunch.
	 */
	crank_random();
	for (cp = buf, i = 0; i < nbytes; i++)
		*cp++ ^= (rand() >> 7) & 0xFF;

	return n != 0;
}


/*
 * Tell source of randomness.
 */
const char *random_tell_source(void)
{
	return "getrandom() function";
}
