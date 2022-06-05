/*
 * This code is in the public domain; do with it what you wish.
 *
 * Copyright (C) 2012 Sami Kerola <kerolasa@iki.fi>
 * Copyright (C) 2012-2020 Karel Zak <kzak@redhat.com>
 */

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/resource.h>
#include <string.h>

#include "fileutils.h"
#include "pathnames.h"

int mkstemp_cloexec(char *template)
{
	int fd, old_flags, errno_save;

	fd = mkstemp(template);
	if (fd < 0)
		return fd;

	old_flags = fcntl(fd, F_GETFD, 0);
	if (old_flags < 0)
		goto unwind;
	if (fcntl(fd, F_SETFD, old_flags | O_CLOEXEC) < 0)
		goto unwind;

	return fd;

unwind:
	errno_save = errno;
	unlink(template);
	close(fd);
	errno = errno_save;

	return -1;
}

/* Create open temporary file in safe way.  Please notice that the
 * file permissions are -rw------- by default. */
int xmkstemp(char **tmpname, const char *dir, const char *prefix)
{
	char *localtmp;
	const char *tmpenv;
	mode_t old_mode;
	int fd, rc;

	/* Some use cases must be capable of being moved atomically
	 * with rename(2), which is the reason why dir is here.  */
	tmpenv = dir ? dir : getenv("TMPDIR");
	if (!tmpenv)
		tmpenv = _PATH_TMP;

	rc = asprintf(&localtmp, "%s/%s.XXXXXX", tmpenv, prefix);
	if (rc < 0)
		return -1;

	old_mode = umask(077);
	fd = mkstemp_cloexec(localtmp);
	umask(old_mode);
	if (fd == -1) {
		free(localtmp);
		localtmp = NULL;
	}
	*tmpname = localtmp;
	return fd;
}

int dup_fd_cloexec(int oldfd, int lowfd)
{
	int fd, flags, errno_save;

#ifdef F_DUPFD_CLOEXEC
	fd = fcntl(oldfd, F_DUPFD_CLOEXEC, lowfd);
	if (fd >= 0)
		return fd;
#endif

	fd = dup(oldfd);
	if (fd < 0)
		return fd;

	flags = fcntl(fd, F_GETFD);
	if (flags < 0)
		goto unwind;
	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0)
		goto unwind;

	return fd;

unwind:
	errno_save = errno;
	close(fd);
	errno = errno_save;

	return -1;
}

/*
 * portable getdtablesize()
 */
unsigned int get_fd_tabsize(void)
{
	int m=0;
	if(m<=0)m=getdtablesize();
	else if(m<=0){
		struct rlimit rl;
		getrlimit(RLIMIT_NOFILE,&rl);
		m=rl.rlim_cur;
	}else if(m<=0)m=sysconf(_SC_OPEN_MAX);
	return m;
}

void ul_close_all_fds(unsigned int first, unsigned int last)
{
	struct dirent *d;
	DIR *dir;

	dir = opendir(_PATH_PROC_FDDIR);
	if (dir) {
		while ((d = readdir(dir))) {
			char *end;
			unsigned int fd;
			int dfd;

			errno = 0;
			fd = strtoul(d->d_name, &end, 10);

			if (errno || end == d->d_name || !end || *end)
				continue;
			dfd = dirfd(dir);
			if (dfd < 0)
				continue;
			if ((unsigned int)dfd == fd)
				continue;
			if (fd < first || last < fd)
				continue;
			close(fd);
		}
		closedir(dir);
	} else {
		unsigned fd, tbsz = get_fd_tabsize();

		for (fd = 0; fd < tbsz; fd++) {
			if (first <= fd && fd <= last)
				close(fd);
		}
	}
}


int ul_mkdir_p(const char *path, mode_t mode)
{
	char *p, *dir;
	int rc = 0;

	if (!path || !*path)
		return -EINVAL;

	dir = p = strdup(path);
	if (!dir)
		return -ENOMEM;

	if (*p == '/')
		p++;

	while (p && *p) {
		char *e = strchr(p, '/');
		if (e)
			*e = '\0';
		if (*p) {
			rc = mkdir(dir, mode);
			if (rc && errno != EEXIST)
				break;
			rc = 0;
		}
		if (!e)
			break;
		*e = '/';
		p = e + 1;
	}

	free(dir);
	return rc;
}

/* returns basename and keeps dirname in the @path, if @path is "/" (root)
 * then returns empty string */
char *stripoff_last_component(char *path)
{
	char *p = path ? strrchr(path, '/') : NULL;

	if (!p)
		return NULL;
	*p = '\0';
	return p + 1;
}

static int copy_file_simple(int from, int to)
{
	ssize_t nr;
	char buf[BUFSIZ];

	while ((nr = read(from, buf, sizeof(buf))) > 0)
		if (write(to, buf, nr) != nr)
			return UL_COPY_WRITE_ERROR;
	if (nr < 0)
		return UL_COPY_READ_ERROR;
	explicit_bzero(buf, sizeof(buf));
	return 0;
}

/* Copies the contents of a file. Returns -1 on read error, -2 on write error. */
int ul_copy_file(int from, int to)
{
	struct stat st;
	ssize_t nw;

	if (fstat(from, &st) == -1)
		return UL_COPY_READ_ERROR;
	if (!S_ISREG(st.st_mode))
		return copy_file_simple(from, to);
	if (sendfile(to, from, NULL, st.st_size) < 0)
		return copy_file_simple(from, to);
	/* ensure we either get an EOF or an error */
	while ((nw = sendfile(to, from, NULL, 16*1024*1024)) != 0)
		if (nw < 0)
			return copy_file_simple(from, to);
	return 0;
}

int ul_reopen(int fd, int flags)
{
	ssize_t ssz;
	char buf[PATH_MAX];
	char fdpath[ sizeof(_PATH_PROC_FDDIR) + 64];

	snprintf(fdpath, sizeof(fdpath), _PATH_PROC_FDDIR "/%d", fd);

	ssz = readlink(fdpath, buf, sizeof(buf) - 1);
	if (ssz < 0)
		return -errno;

	buf[ssz] = '\0';

	return open(buf, flags);
}
