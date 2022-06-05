
#define _GNU_SOURCE
#include "fdiskP.h"
#include "pathnames.h"
#include "canonicalize.h"

#include <ctype.h>

/**
 * SECTION: utils
 * @title: Utils
 * @short_description: misc fdisk functions
 */

static int read_from_device(struct fdisk_context *cxt,
		unsigned char *buf,
		uintmax_t start, size_t size)
{
	ssize_t r;

	r = lseek(cxt->dev_fd, start, SEEK_SET);
	if (r == -1)
		return -errno;

	r = read(cxt->dev_fd, buf, size);
	if (r < 0 || (size_t)r != size) {
		if (!errno)
			errno = EINVAL;	/* probably too small file/device */
		return -errno;
	}

	return 0;
}


/*
 * Zeros in-memory first sector buffer
 */
int fdisk_init_firstsector_buffer(struct fdisk_context *cxt,
				  unsigned int protect_off,
				  unsigned int protect_size)
{
	if (!cxt)
		return -EINVAL;

	if (!cxt->firstsector || cxt->firstsector_bufsz != cxt->sector_size) {
		/* Let's allocate a new buffer if no allocated yet, or the
		 * current buffer has incorrect size */
		if (!cxt->parent || cxt->parent->firstsector != cxt->firstsector)
			free(cxt->firstsector);

		cxt->firstsector = calloc(1, cxt->sector_size);
		if (!cxt->firstsector)
			return -ENOMEM;

		cxt->firstsector_bufsz = cxt->sector_size;
		return 0;
	}

	memset(cxt->firstsector, 0, cxt->firstsector_bufsz);

	if (protect_size) {
		/*
		 * It would be possible to reuse data from cxt->firstsector
		 * (call memset() for non-protected area only) and avoid one
		 * read() from the device, but it seems like a too fragile
		 * solution as we have no clue about stuff in the buffer --
		 * maybe it was already modified. Let's re-read from the device
		 * to be sure.			-- kzak 13-Apr-2015
		 */
		read_from_device(cxt, cxt->firstsector, protect_off, protect_size);
	}
	return 0;
}

int fdisk_read_firstsector(struct fdisk_context *cxt)
{
	int rc;

	rc = fdisk_init_firstsector_buffer(cxt, 0, 0);
	if (rc)
		return rc;

	return  read_from_device(cxt, cxt->firstsector, 0, cxt->sector_size);
}

/**
 * fdisk_partname:
 * @dev: device name
 * @partno: partition name
 *
 * Return: allocated buffer with partition name, use free() to deallocate.
 */
char *fdisk_partname(const char *dev, size_t partno)
{
	char *res = NULL;
	const char *p = "";
	char *dev_mapped = NULL;
	int w = 0;

	if (!dev || !*dev) {
		if (asprintf(&res, "%zd", partno) > 0)
			return res;
		return NULL;
	}

	/* It is impossible to predict /dev/dm-N partition names. */
	if (strncmp(dev, "/dev/dm-", sizeof("/dev/dm-") - 1) == 0) {
		dev_mapped = canonicalize_dm_name (dev + 5);
		if (dev_mapped)
			dev = dev_mapped;
	}

	w = strlen(dev);
	if (isdigit(dev[w - 1]))
#ifdef __GNU__
		p = "s";
#else
		p = "p";
#endif

	/* devfs kludge - note: fdisk partition names are not supposed
	   to equal kernel names, so there is no reason to do this */
	if (endswith(dev, "disc")) {
		w -= 4;
		p = "part";
	}

	/* udev names partitions by appending -partN
	   e.g. ata-SAMSUNG_SV8004H_0357J1FT712448-part1
	   multipath-tools kpartx.rules also append -partN */
	if ((strncmp(dev, _PATH_DEV_BYID, sizeof(_PATH_DEV_BYID) - 1) == 0) ||
	     strncmp(dev, _PATH_DEV_BYPATH, sizeof(_PATH_DEV_BYPATH) - 1) == 0 ||
	     strncmp(dev, _PATH_DEV_MAPPER, sizeof(_PATH_DEV_MAPPER) - 1) == 0) {

		/* check for <name><partno>, e.g. mpatha1 */
		if (asprintf(&res, "%.*s%zu", w, dev, partno) <= 0)
			res = NULL;
		if (res && access(res, F_OK) == 0)
			goto done;

		free(res);

		/* check for partition separator "p" */
		if (asprintf(&res, "%.*sp%zu", w, dev, partno) <= 0)
			res = NULL;
		if (res && access(res, F_OK) == 0)
			goto done;

		free(res);

		/* otherwise, default to "-path" */
		p = "-part";
	}

	if (asprintf(&res, "%.*s%s%zu", w, dev, p, partno) <= 0)
		res = NULL;
done:
	free(dev_mapped);
	return res;
}
