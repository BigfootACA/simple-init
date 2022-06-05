/*
 * save.c - write the cache struct to disk
 *
 * Copyright (C) 2001 by Andreas Dilger
 * Copyright (C) 2003 Theodore Ts'o
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 * %End-Header%
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "closestream.h"
#include "fileutils.h"
#include "blkidP.h"


static void save_quoted(const char *data, FILE *file)
{
	const char *p;

	fputc('"', file);
	for (p = data; p && *p; p++) {
		if ((unsigned char) *p == 0x22 ||		/* " */
		    (unsigned char) *p == 0x5c)			/* \ */
			fputc('\\', file);

		fputc(*p, file);
	}
	fputc('"', file);
}
static int save_dev(blkid_dev dev, FILE *file)
{
	struct list_head *p;

	if (!dev || dev->bid_name[0] != '/')
		return 0;

	fprintf(file, "<device DEVNO=\"0x%04lx\" TIME=\"%lld.%lld\"",
			(unsigned long) dev->bid_devno,
			(long long) dev->bid_time,
			(long long) dev->bid_utime);

	if (dev->bid_pri)
		fprintf(file, " PRI=\"%d\"", dev->bid_pri);

	list_for_each(p, &dev->bid_tags) {
		blkid_tag tag = list_entry(p, struct blkid_struct_tag, bit_tags);

		fputc(' ', file);			/* space between tags */
		fputs(tag->bit_name, file);		/* tag NAME */
		fputc('=', file);			/* separator between NAME and VALUE */
		save_quoted(tag->bit_val, file);	/* tag "VALUE" */
	}
	fprintf(file, ">%s</device>\n", dev->bid_name);

	return 0;
}

/*
 * Write out the cache struct to the cache file on disk.
 */
int blkid_flush_cache(blkid_cache cache)
{
	struct list_head *p;
	char *tmp = NULL;
	char *opened = NULL;
	char *filename;
	FILE *file = NULL;
	int fd, ret = 0;
	struct stat st;

	if (list_empty(&cache->bic_devs) ||
	    !(cache->bic_flags & BLKID_BIC_FL_CHANGED)) {
		return 0;
	}

	filename = cache->bic_filename ? cache->bic_filename :
					 blkid_get_cache_filename(NULL);
	if (!filename)
		return -BLKID_ERR_PARAM;

	if (strncmp(filename,
	    BLKID_RUNTIME_DIR "/", sizeof(BLKID_RUNTIME_DIR)) == 0) {

		/* default destination, create the directory if necessary */
		if (stat(BLKID_RUNTIME_DIR, &st)
		    && errno == ENOENT
		    && mkdir(BLKID_RUNTIME_DIR, S_IWUSR|
						S_IRUSR|S_IRGRP|S_IROTH|
						S_IXUSR|S_IXGRP|S_IXOTH) != 0
		    && errno != EEXIST) {
			return 0;
		}
	}

	/* If we can't write to the cache file, then don't even try */
	if (((ret = stat(filename, &st)) < 0 && errno != ENOENT) ||
	    (ret == 0 && access(filename, W_OK) < 0)) {
		return 0;
	}

	/*
	 * Try and create a temporary file in the same directory so
	 * that in case of error we don't overwrite the cache file.
	 * If the cache file doesn't yet exist, it isn't a regular
	 * file (e.g. /dev/null or a socket), or we couldn't create
	 * a temporary file then we open it directly.
	 */
	if (ret == 0 && S_ISREG(st.st_mode)) {
		size_t len = strlen(filename) + 8;
		tmp = malloc(len);
		if (tmp) {
			snprintf(tmp, len, "%s-XXXXXX", filename);
			fd = mkstemp_cloexec(tmp);
			if (fd >= 0) {
				if (fchmod(fd, 0644) == 0&&(file = fdopen(fd, "we")))
					opened = tmp;
				if (!file)
					close(fd);
			}
		}
	}

	if (!file) {
		file = fopen(filename, "we");
		opened = filename;
	}

	if (!file) {
		ret = errno;
		goto errout;
	}

	list_for_each(p, &cache->bic_devs) {
		blkid_dev dev = list_entry(p, struct blkid_struct_dev, bid_devs);
		if (!dev->bid_type || (dev->bid_flags & BLKID_BID_FL_REMOVABLE))
			continue;
		if ((ret = save_dev(dev, file)) < 0)
			break;
	}

	if (ret >= 0) {
		cache->bic_flags &= ~BLKID_BIC_FL_CHANGED;
		ret = 1;
	}

	close_stream(file);

	if (opened != filename) {
		if (ret < 0) {
			unlink(opened);
		} else {
			char *backup;
			size_t len = strlen(filename) + 5;

			backup = malloc(len);
			if (backup) {
				snprintf(backup, len, "%s.old", filename);
				unlink(backup);
				link(filename, backup);
				free(backup);
			}
			if (rename(opened, filename)) {
				ret = errno;
			}
		}
	}

errout:
	free(tmp);
	if (filename != cache->bic_filename)
		free(filename);
	return ret;
}
