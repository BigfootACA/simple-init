/*
 * Simple functions to access files. Paths can be globally prefixed to read
 * data from an alternative source (e.g. a /proc dump for regression tests).
 *
 * The paths is possible to format by printf-like way for functions with "f"
 * postfix in the name (e.g. readf, openf, ... ul_path_readf_u64()).
 *
 * The ul_path_read_* API is possible to use without path_cxt handler. In this
 * case is not possible to use global prefix and printf-like formatting.
 *
 * No copyright is claimed.  This code is in the public domain; do with
 * it what you wish.
 *
 * Written by Karel Zak <kzak@redhat.com> [February 2018]
 */

#define _GNU_SOURCE
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/sysmacros.h>
#include <errno.h>

#include "fileutils.h"
#include "path.h"
#include "debug.h"

struct path_cxt *ul_new_path(const char *dir, ...)
{
	struct path_cxt *pc = calloc(1, sizeof(*pc));

	if (!pc)
		return NULL;

	pc->refcount = 1;
	pc->dir_fd = -1;

	if (dir) {
		int rc;
		va_list ap;

		va_start(ap, dir);
		rc = vasprintf(&pc->dir_path, dir, ap);
		va_end(ap);

		if (rc < 0 || !pc->dir_path)
			goto fail;
	}
	return pc;
fail:
	ul_unref_path(pc);
	return NULL;
}

void ul_ref_path(struct path_cxt *pc)
{
	if (pc)
		pc->refcount++;
}

void ul_unref_path(struct path_cxt *pc)
{
	if (!pc)
		return;

	pc->refcount--;

	if (pc->refcount <= 0) {
		if (pc->dialect)
			pc->free_dialect(pc);
		ul_path_close_dirfd(pc);
		free(pc->dir_path);
		free(pc->prefix);
		free(pc);
	}
}

int ul_path_set_prefix(struct path_cxt *pc, const char *prefix)
{
	char *p = NULL;

	if (prefix) {
		p = strdup(prefix);
		if (!p)
			return -ENOMEM;
	}

	free(pc->prefix);
	pc->prefix = p;
	return 0;
}

const char *ul_path_get_prefix(struct path_cxt *pc)
{
	return pc ? pc->prefix : NULL;
}

int ul_path_set_dir(struct path_cxt *pc, const char *dir)
{
	char *p = NULL;

	if (dir) {
		p = strdup(dir);
		if (!p)
			return -ENOMEM;
	}

	if (pc->dir_fd >= 0) {
		close(pc->dir_fd);
		pc->dir_fd = -1;
	}

	free(pc->dir_path);
	pc->dir_path = p;
	return 0;
}

const char *ul_path_get_dir(struct path_cxt *pc)
{
	return pc ? pc->dir_path : NULL;
}

int ul_path_set_dialect(struct path_cxt *pc, void *data, void free_data(struct path_cxt *))
{
	pc->dialect = data;
	pc->free_dialect = free_data;
	return 0;
}

void *ul_path_get_dialect(struct path_cxt *pc)
{
	return pc ? pc->dialect : NULL;
}

int ul_path_set_enoent_redirect(struct path_cxt *pc, int (*func)(struct path_cxt *, const char *, int *))
{
	pc->redirect_on_enoent = func;
	return 0;
}

static const char *get_absdir(struct path_cxt *pc)
{
	int rc;
	const char *dirpath;

	if (!pc->prefix)
		return pc->dir_path;

	dirpath = pc->dir_path;
	if (!dirpath)
		return pc->prefix;
	if (*dirpath == '/')
		dirpath++;

	rc = snprintf(pc->path_buffer, sizeof(pc->path_buffer), "%s/%s", pc->prefix, dirpath);
	if (rc < 0)
		return NULL;
	if ((size_t)rc >= sizeof(pc->path_buffer)) {
		errno = ENAMETOOLONG;
		return NULL;
	}

	return pc->path_buffer;
}

int ul_path_is_accessible(struct path_cxt *pc)
{
	const char *path;

	if (pc->dir_fd >= 0)
		return 1;

	path = get_absdir(pc);
	if (!path)
		return 0;
	return access(path, F_OK) == 0;
}

int ul_path_get_dirfd(struct path_cxt *pc)
{
	if (pc->dir_fd < 0) {
		const char *path = get_absdir(pc);
		if (!path)
			return -errno;

		pc->dir_fd = open(path, O_RDONLY|O_CLOEXEC);
	}

	return pc->dir_fd;
}

/* Note that next ul_path_get_dirfd() will reopen the directory */
void ul_path_close_dirfd(struct path_cxt *pc)
{
	if (pc->dir_fd >= 0) {
		close(pc->dir_fd);
		pc->dir_fd = -1;
	}
}

int ul_path_isopen_dirfd(struct path_cxt *pc)
{
	return pc && pc->dir_fd >= 0;
}

static const char *ul_path_mkpath(struct path_cxt *pc, const char *path, va_list ap)
{
	int rc;

	errno = 0;

	rc = vsnprintf(pc->path_buffer, sizeof(pc->path_buffer), path, ap);
	if (rc < 0) {
		if (!errno)
			errno = EINVAL;
		return NULL;
	}

	if ((size_t)rc >= sizeof(pc->path_buffer)) {
		errno = ENAMETOOLONG;
		return NULL;
	}

	return pc->path_buffer;
}

char *ul_path_get_abspath(struct path_cxt *pc, char *buf, size_t bufsz, const char *path, ...)
{
	if (path) {
		int rc;
		va_list ap;
		const char *tail = NULL, *dirpath = pc->dir_path;

		va_start(ap, path);
		tail = ul_path_mkpath(pc, path, ap);
		va_end(ap);

		if (dirpath && *dirpath == '/')
			dirpath++;
		if (tail && *tail == '/')
			tail++;

		rc = snprintf(buf, bufsz, "%s/%s/%s",
				pc->prefix ? pc->prefix : "",
				dirpath ? dirpath : "",
				tail ? tail : "");

		if ((size_t)rc >= bufsz) {
			errno = ENAMETOOLONG;
			return NULL;
		}
	} else {
		const char *tmp = get_absdir(pc);

		if (!tmp)
			return NULL;
		strncpy(buf, tmp, bufsz);
	}

	return buf;
}


int ul_path_access(struct path_cxt *pc, int mode, const char *path)
{
	int rc;

	if (!pc) {
		rc = access(path, mode);
	} else {
		int dir = ul_path_get_dirfd(pc);
		if (dir < 0)
			return dir;
		if (*path == '/')
			path++;

		rc = faccessat(dir, path, mode, 0);

		if (rc && errno == ENOENT
		    && pc->redirect_on_enoent
		    && pc->redirect_on_enoent(pc, path, &dir) == 0)
			rc = faccessat(dir, path, mode, 0);
	}
	return rc;
}

int ul_path_accessf(struct path_cxt *pc, int mode, const char *path, ...)
{
	va_list ap;
	const char *p;

	va_start(ap, path);
	p = ul_path_mkpath(pc, path, ap);
	va_end(ap);

	return !p ? -errno : ul_path_access(pc, mode, p);
}

int ul_path_stat(struct path_cxt *pc, struct stat *sb, int flags, const char *path)
{
	int rc;

	if (!pc) {
		rc = path ? stat(path, sb) : -EINVAL;
	} else {
		int dir = ul_path_get_dirfd(pc);
		if (dir < 0)
			return dir;
		if (path) {
			if  (*path == '/')
				path++;
			rc = fstatat(dir, path, sb, flags);

		} else
			rc = fstat(dir, sb);	/* dir itself */

		if (rc && errno == ENOENT
		    && path
		    && pc->redirect_on_enoent
		    && pc->redirect_on_enoent(pc, path, &dir) == 0)
			rc = fstatat(dir, path, sb, 0);
	}
	return rc;
}

int ul_path_open(struct path_cxt *pc, int flags, const char *path)
{
	int fd;

	if (!path)
		return -EINVAL;
	if (!pc) {
		fd = open(path, flags);
	} else {
		int dir = ul_path_get_dirfd(pc);
		if (dir < 0)
			return dir;

		if (*path == '/')
			path++;

		fd = openat(dir, path, flags);

		if (fd < 0 && errno == ENOENT
		    && pc->redirect_on_enoent
		    && pc->redirect_on_enoent(pc, path, &dir) == 0)
			fd = openat(dir, path, flags);
	}
	return fd;
}

int ul_path_vopenf(struct path_cxt *pc, int flags, const char *path, va_list ap)
{
	const char *p = ul_path_mkpath(pc, path, ap);

	return !p ? -errno : ul_path_open(pc, flags, p);
}

int ul_path_openf(struct path_cxt *pc, int flags, const char *path, ...)
{
	va_list ap;
	int rc;

	va_start(ap, path);
	rc = ul_path_vopenf(pc, flags, path, ap);
	va_end(ap);

	return rc;
}

/*
 * Maybe stupid, but good enough ;-)
 */
static int mode2flags(const char *mode)
{
	int flags = 0;
	const char *p;

	for (p = mode; p && *p; p++) {
		if (*p == 'r' && *(p + 1) == '+')
			flags |= O_RDWR;
		else if (*p == 'r')
			flags |= O_RDONLY;

		else if (*p == 'w' && *(p + 1) == '+')
			flags |= O_RDWR | O_TRUNC;
		else if (*p == 'w')
			flags |= O_WRONLY | O_TRUNC;

		else if (*p == 'a' && *(p + 1) == '+')
			flags |= O_RDWR | O_APPEND;
		else if (*p == 'a')
			flags |= O_WRONLY | O_APPEND;
#ifdef O_CLOEXEC
		else if (*p == 'e')
			flags |= O_CLOEXEC;
#endif
	}

	return flags;
}

FILE *ul_path_fopen(struct path_cxt *pc, const char *mode, const char *path)
{
	int flags = mode2flags(mode);
	int fd = ul_path_open(pc, flags, path);

	if (fd < 0)
		return NULL;

	return fdopen(fd, mode);
}


FILE *ul_path_vfopenf(struct path_cxt *pc, const char *mode, const char *path, va_list ap)
{
	const char *p = ul_path_mkpath(pc, path, ap);

	return !p ? NULL : ul_path_fopen(pc, mode, p);
}

FILE *ul_path_fopenf(struct path_cxt *pc, const char *mode, const char *path, ...)
{
	FILE *f;
	va_list ap;

	va_start(ap, path);
	f = ul_path_vfopenf(pc, mode, path, ap);
	va_end(ap);

	return f;
}

/*
 * Open directory @path in read-onl mode. If the path is NULL then duplicate FD
 * to the directory addressed by @pc.
 */
DIR *ul_path_opendir(struct path_cxt *pc, const char *path)
{
	DIR *dir;
	int fd = -1;

	if (path)
		fd = ul_path_open(pc, O_RDONLY|O_CLOEXEC, path);
	else if (pc->dir_path) {
		int dirfd;

		dirfd = ul_path_get_dirfd(pc);
		if (dirfd >= 0)
			fd = dup_fd_cloexec(dirfd, STDERR_FILENO + 1);
	}

	if (fd < 0)
		return NULL;

	dir = fdopendir(fd);
	if (!dir) {
		close(fd);
		return NULL;
	}
	if (!path)
		 rewinddir(dir);
	return dir;
}


/*
 * Open directory @path in read-onl mode. If the path is NULL then duplicate FD
 * to the directory addressed by @pc.
 */
DIR *ul_path_vopendirf(struct path_cxt *pc, const char *path, va_list ap)
{
	const char *p = ul_path_mkpath(pc, path, ap);

	return !p ? NULL : ul_path_opendir(pc, p);
}

/*
 * Open directory @path in read-onl mode. If the path is NULL then duplicate FD
 * to the directory addressed by @pc.
 */
DIR *ul_path_opendirf(struct path_cxt *pc, const char *path, ...)
{
	va_list ap;
	DIR *dir;

	va_start(ap, path);
	dir = ul_path_vopendirf(pc, path, ap);
	va_end(ap);

	return dir;
}

/*
 * If @path is NULL then readlink is called on @pc directory.
 */
ssize_t ul_path_readlink(struct path_cxt *pc, char *buf, size_t bufsiz, const char *path)
{
	int dirfd;
	ssize_t ssz;

	if (!path) {
		const char *p = get_absdir(pc);
		if (!p)
			return -errno;
		ssz = readlink(p, buf, bufsiz - 1);
	} else {
		dirfd = ul_path_get_dirfd(pc);
		if (dirfd < 0)
			return dirfd;

		if (*path == '/')
			path++;

		ssz = readlinkat(dirfd, path, buf, bufsiz - 1);
	}

	if (ssz >= 0)
		buf[ssz] = '\0';
	return ssz;
}

/*
 * If @path is NULL then readlink is called on @pc directory.
 */
ssize_t ul_path_readlinkf(struct path_cxt *pc, char *buf, size_t bufsiz, const char *path, ...)
{
	const char *p;
	va_list ap;

	va_start(ap, path);
	p = ul_path_mkpath(pc, path, ap);
	va_end(ap);

	return !p ? -errno : ul_path_readlink(pc, buf, bufsiz, p);
}

int ul_path_read(struct path_cxt *pc, char *buf, size_t len, const char *path)
{
	int rc, errsv;
	int fd;

	fd = ul_path_open(pc, O_RDONLY|O_CLOEXEC, path);
	if (fd < 0)
		return -errno;

	rc = read(fd, buf, len) != (ssize_t)len;

	errsv = errno;
	close(fd);
	errno = errsv;
	return rc;
}

int ul_path_vreadf(struct path_cxt *pc, char *buf, size_t len, const char *path, va_list ap)
{
	const char *p = ul_path_mkpath(pc, path, ap);

	return !p ? -errno : ul_path_read(pc, buf, len, p);
}

int ul_path_readf(struct path_cxt *pc, char *buf, size_t len, const char *path, ...)
{
	va_list ap;
	int rc;

	va_start(ap, path);
	rc = ul_path_vreadf(pc, buf, len, path, ap);
	va_end(ap);

	return rc;
}


/*
 * Returns newly allocated buffer with data from file. Maximal size is BUFSIZ
 * (send patch if you need something bigger;-)
 *
 * Returns size of the string without \0, nothing is allocated if returns <= 0.
 */
int ul_path_read_string(struct path_cxt *pc, char **str, const char *path)
{
	char buf[BUFSIZ];
	int rc;

	if (!str)
		return -EINVAL;

	*str = NULL;
	rc = ul_path_read(pc, buf, sizeof(buf) - 1, path);
	if (rc < 0)
		return rc;

	/* Remove tailing newline (usual in sysfs) */
	if (rc > 0 && *(buf + rc - 1) == '\n')
		--rc;
	if (rc == 0)
		return 0;

	buf[rc] = '\0';
	*str = strdup(buf);
	if (!*str)
		rc = -ENOMEM;

	return rc;
}

int ul_path_readf_string(struct path_cxt *pc, char **str, const char *path, ...)
{
	const char *p;
	va_list ap;

	va_start(ap, path);
	p = ul_path_mkpath(pc, path, ap);
	va_end(ap);

	return !p ? -errno : ul_path_read_string(pc, str, p);
}

int ul_path_read_buffer(struct path_cxt *pc, char *buf, size_t bufsz, const char *path)
{
	int rc = ul_path_read(pc, buf, bufsz - 1, path);

	if (rc == 0)
		buf[0] = '\0';

	else if (rc > 0) {
		/* Remove tailing newline (usual in sysfs) */
		if (*(buf + rc - 1) == '\n')
			buf[--rc] = '\0';
		else
			buf[rc - 1] = '\0';
	}

	return rc;
}

int ul_path_readf_buffer(struct path_cxt *pc, char *buf, size_t bufsz, const char *path, ...)
{
	const char *p;
	va_list ap;

	va_start(ap, path);
	p = ul_path_mkpath(pc, path, ap);
	va_end(ap);

	return !p ? -errno : ul_path_read_buffer(pc, buf, bufsz, p);
}

int ul_path_scanf(struct path_cxt *pc, const char *path, const char *fmt, ...)
{
	FILE *f;
	va_list fmt_ap;
	int rc;

	f = ul_path_fopen(pc, "re", path);
	if (!f)
		return -EINVAL;

	va_start(fmt_ap, fmt);
	rc = vfscanf(f, fmt, fmt_ap);
	va_end(fmt_ap);

	fclose(f);
	return rc;
}

int ul_path_scanff(struct path_cxt *pc, const char *path, va_list ap, const char *fmt, ...)
{
	FILE *f;
	va_list fmt_ap;
	int rc;

	f = ul_path_vfopenf(pc, "re", path, ap);
	if (!f)
		return -EINVAL;

	va_start(fmt_ap, fmt);
	rc = vfscanf(f, fmt, fmt_ap);
	va_end(fmt_ap);

	fclose(f);
	return rc;
}


int ul_path_read_s64(struct path_cxt *pc, int64_t *res, const char *path)
{
	int64_t x = 0;
	int rc;

	rc = ul_path_scanf(pc, path, "%"SCNd64, &x);
	if (rc != 1)
		return -1;
	if (res)
		*res = x;
	return 0;
}

int ul_path_readf_s64(struct path_cxt *pc, int64_t *res, const char *path, ...)
{
	const char *p;
	va_list ap;

	va_start(ap, path);
	p = ul_path_mkpath(pc, path, ap);
	va_end(ap);

	return !p ? -errno : ul_path_read_s64(pc, res, p);
}

int ul_path_read_u64(struct path_cxt *pc, uint64_t *res, const char *path)
{
	uint64_t x = 0;
	int rc;

	rc = ul_path_scanf(pc, path, "%"SCNu64, &x);
	if (rc != 1)
		return -1;
	if (res)
		*res = x;
	return 0;
}

int ul_path_readf_u64(struct path_cxt *pc, uint64_t *res, const char *path, ...)
{
	const char *p;
	va_list ap;

	va_start(ap, path);
	p = ul_path_mkpath(pc, path, ap);
	va_end(ap);

	return !p ? -errno : ul_path_read_u64(pc, res, p);
}

int ul_path_read_s32(struct path_cxt *pc, int *res, const char *path)
{
	int rc, x = 0;

	rc = ul_path_scanf(pc, path, "%d", &x);
	if (rc != 1)
		return -1;
	if (res)
		*res = x;
	return 0;
}

int ul_path_readf_s32(struct path_cxt *pc, int *res, const char *path, ...)
{
	const char *p;
	va_list ap;

	va_start(ap, path);
	p = ul_path_mkpath(pc, path, ap);
	va_end(ap);

	return !p ? -errno : ul_path_read_s32(pc, res, p);
}

int ul_path_read_u32(struct path_cxt *pc, unsigned int *res, const char *path)
{
	int rc;
	unsigned int x = 0;

	rc = ul_path_scanf(pc, path, "%u", &x);
	if (rc != 1)
		return -1;
	if (res)
		*res = x;
	return 0;
}

int ul_path_readf_u32(struct path_cxt *pc, unsigned int *res, const char *path, ...)
{
	const char *p;
	va_list ap;

	va_start(ap, path);
	p = ul_path_mkpath(pc, path, ap);
	va_end(ap);

	return !p ? -errno : ul_path_read_u32(pc, res, p);
}

int ul_path_read_majmin(struct path_cxt *pc, dev_t *res, const char *path)
{
	int rc, maj = 0, min = 0;

	rc = ul_path_scanf(pc, path, "%d:%d", &maj, &min);
	if (rc != 2)
		return -1;
	if (res)
		*res = makedev(maj, min);
	return 0;
}

int ul_path_readf_majmin(struct path_cxt *pc, dev_t *res, const char *path, ...)
{
	const char *p;
	va_list ap;

	va_start(ap, path);
	p = ul_path_mkpath(pc, path, ap);
	va_end(ap);

	return !p ? -errno : ul_path_read_majmin(pc, res, p);
}

int ul_path_write_string(struct path_cxt *pc, const char *str, const char *path)
{
	int rc, errsv;
	int fd;

	fd = ul_path_open(pc, O_WRONLY|O_CLOEXEC, path);
	if (fd < 0)
		return -errno;

	ssize_t len = (ssize_t)strlen(str);
	rc = write(fd, str, len) != len;

	errsv = errno;
	close(fd);
	errno = errsv;
	return rc;
}

int ul_path_writef_string(struct path_cxt *pc, const char *str, const char *path, ...)
{
	const char *p;
	va_list ap;

	va_start(ap, path);
	p = ul_path_mkpath(pc, path, ap);
	va_end(ap);

	return !p ? -errno : ul_path_write_string(pc, str, p);
}

int ul_path_write_s64(struct path_cxt *pc, int64_t num, const char *path)
{
	char buf[64];
	int rc, errsv;
	int fd, len;

	fd = ul_path_open(pc, O_WRONLY|O_CLOEXEC, path);
	if (fd < 0)
		return -errno;

	len = snprintf(buf, sizeof(buf), "%" PRId64, num);
	if (len < 0 || (size_t) len >= sizeof(buf))
		rc = len < 0 ? -errno : -E2BIG;
	else
		rc = write(fd, buf, len) != (ssize_t)len;

	errsv = errno;
	close(fd);
	errno = errsv;
	return rc;
}

int ul_path_write_u64(struct path_cxt *pc, uint64_t num, const char *path)
{
	char buf[64];
	int rc, errsv;
	int fd, len;

	fd = ul_path_open(pc, O_WRONLY|O_CLOEXEC, path);
	if (fd < 0)
		return -errno;

	len = snprintf(buf, sizeof(buf), "%" PRIu64, num);
	if (len < 0 || (size_t) len >= sizeof(buf))
		rc = len < 0 ? -errno : -E2BIG;
	else
		rc = write(fd, buf, len) != (ssize_t)len;

	errsv = errno;
	close(fd);
	errno = errsv;
	return rc;
}

int ul_path_writef_u64(struct path_cxt *pc, uint64_t num, const char *path, ...)
{
	const char *p;
	va_list ap;

	va_start(ap, path);
	p = ul_path_mkpath(pc, path, ap);
	va_end(ap);

	return !p ? -errno : ul_path_write_u64(pc, num, p);

}

int ul_path_count_dirents(struct path_cxt *pc, const char *path)
{
	DIR *dir;
	int r = 0;

	dir = ul_path_opendir(pc, path);
	if (!dir)
		return 0;

	while (readdir(dir)) r++;

	closedir(dir);
	return r;
}

int ul_path_countf_dirents(struct path_cxt *pc, const char *path, ...)
{
	const char *p;
	va_list ap;

	va_start(ap, path);
	p = ul_path_mkpath(pc, path, ap);
	va_end(ap);

	return !p ? -errno : ul_path_count_dirents(pc, p);
}

/* first call (when @sub is NULL) opens the directory, last call closes the diretory */
int ul_path_next_dirent(struct path_cxt *pc, DIR **sub, const char *dirname, struct dirent **d)
{
	if (!pc || !sub || !d)
		return -EINVAL;

	if (!*sub) {
		*sub = ul_path_opendir(pc, dirname);
		if (!*sub)
			return -errno;
	}

	*d = readdir(*sub);
	if (*d)
		return 0;

	closedir(*sub);
	*sub = NULL;
	return 1;
}

/*
 * Like fopen() but, @path is always prefixed by @prefix. This function is
 * useful in case when ul_path_* API is overkill.
 */
FILE *ul_prefix_fopen(const char *prefix, const char *path, const char *mode)
{
	char buf[PATH_MAX];

	if (!path)
		return NULL;
	if (!prefix)
		return fopen(path, mode);
	if (*path == '/')
		path++;

	snprintf(buf, sizeof(buf), "%s/%s", prefix, path);
	return fopen(buf, mode);
}
