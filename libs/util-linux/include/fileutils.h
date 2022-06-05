#ifndef UTIL_LINUX_FILEUTILS
#define UTIL_LINUX_FILEUTILS

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

extern int mkstemp_cloexec(char *template);

extern int xmkstemp(char **tmpname, const char *dir, const char *prefix);

static inline FILE *xfmkstemp(char **tmpname, const char *dir, const char *prefix)
{
	int fd;
	FILE *ret;

	fd = xmkstemp(tmpname, dir, prefix);
	if (fd == -1)
		return NULL;

	if (!(ret = fdopen(fd, "w+e"))) {
		close(fd);
		return NULL;
	}
	return ret;
}

static inline int is_same_inode(const int fd, const struct stat *st)
{
	struct stat f;

	if (fstat(fd, &f) < 0)
		return 0;
	else if (f.st_dev != st->st_dev || f.st_ino != st->st_ino)
		return 0;
	return 1;
}

extern int dup_fd_cloexec(int oldfd, int lowfd);
extern unsigned int get_fd_tabsize(void);

extern int ul_mkdir_p(const char *path, mode_t mode);
extern char *stripoff_last_component(char *path);

extern void ul_close_all_fds(unsigned int first, unsigned int last);

#define UL_COPY_READ_ERROR (-1)
#define UL_COPY_WRITE_ERROR (-2)
int ul_copy_file(int from, int to);


extern int ul_reopen(int fd, int flags);

#endif /* UTIL_LINUX_FILEUTILS */
