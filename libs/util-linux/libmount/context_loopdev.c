/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libmount from util-linux project.
 *
 * Copyright (C) 2011-2018 Karel Zak <kzak@redhat.com>
 *
 * libmount is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

/*
 * DOCS: - "lo@" prefix for fstype is unsupported
 */

#include <libblkid/blkid.h>
#include <stdbool.h>

#include "mountP.h"
#include "loopdev.h"
#include "linux_version.h"


int mnt_context_is_loopdev(struct libmnt_context *cxt)
{
	const char *type, *src;

	if (!cxt->fs)
		return 0;
	src = mnt_fs_get_srcpath(cxt->fs);
	if (!src)
		return 0;		/* backing file not set */

	if (cxt->user_mountflags & (MNT_MS_LOOP |
				    MNT_MS_OFFSET |
				    MNT_MS_SIZELIMIT)) {

		return 1;
	}

	if ((cxt->mountflags & (MS_BIND | MS_MOVE))
	    || mnt_context_propagation_only(cxt))
		return 0;

	/* Automatically create a loop device from a regular file if a
	 * filesystem is not specified or the filesystem is known for libblkid
	 * (these filesystems work with block devices only). The file size
	 * should be at least 1KiB, otherwise we will create an empty loopdev with
	 * no mountable filesystem...
	 *
	 * Note that there is no restriction (on kernel side) that would prevent a regular
	 * file as a mount(2) source argument. A filesystem that is able to mount
	 * regular files could be implemented.
	 */
	type = mnt_fs_get_fstype(cxt->fs);

	if (mnt_fs_is_regularfs(cxt->fs) &&
	    (!type || strcmp(type, "auto") == 0 || blkid_known_fstype(type))) {
		struct stat st;

		if (stat(src, &st) == 0 && S_ISREG(st.st_mode) &&
		    st.st_size > 1024) {
			cxt->user_mountflags |= MNT_MS_LOOP;
			mnt_optstr_append_option(&cxt->fs->user_optstr, "loop", NULL);
			return 1;
		}
	}

	return 0;
}


/* Check if there already exists a mounted loop device on the mountpoint node
 * with the same parameters.
 */
static int __attribute__((nonnull))
is_mounted_same_loopfile(struct libmnt_context *cxt,
				    const char *target,
				    const char *backing_file,
				    uint64_t offset)
{
	struct libmnt_table *tb;
	struct libmnt_iter itr;
	struct libmnt_fs *fs;
	struct libmnt_cache *cache;
	const char *bf;
	int rc = 0;
	struct libmnt_ns *ns_old;

	if (mnt_context_get_mtab(cxt, &tb))
		return 0;

	ns_old = mnt_context_switch_target_ns(cxt);
	if (!ns_old)
		return -MNT_ERR_NAMESPACE;

	cache = mnt_context_get_cache(cxt);
	mnt_reset_iter(&itr, MNT_ITER_BACKWARD);

	bf = cache ? mnt_resolve_path(backing_file, cache) : backing_file;

	/* Search for a mountpoint node in mtab, proceed if any of these have the
	 * loop option set or the device is a loop device
	 */
	while (rc == 0 && mnt_table_next_fs(tb, &itr, &fs) == 0) {
		const char *src = mnt_fs_get_source(fs);
		const char *opts = mnt_fs_get_user_options(fs);
		char *val;
		size_t len;

		if (!src || !mnt_fs_match_target(fs, target, cache))
			continue;

		rc = 0;

		if (strncmp(src, "/dev/loop", 9) == 0) {
			rc = loopdev_is_used((char *) src, bf, offset, 0, LOOPDEV_FL_OFFSET);

		} else if (opts && (cxt->user_mountflags & MNT_MS_LOOP) &&
		    mnt_optstr_get_option(opts, "loop", &val, &len) == 0 && val) {

			val = strndup(val, len);
			rc = loopdev_is_used((char *) val, bf, offset, 0, LOOPDEV_FL_OFFSET);
			free(val);
		}
	}

	if (!mnt_context_switch_ns(cxt, ns_old))
		return -MNT_ERR_NAMESPACE;
	return rc;
}

int mnt_context_setup_loopdev(struct libmnt_context *cxt)
{
	const char *backing_file, *optstr, *loopdev = NULL;
	char *val = NULL, *loopval = NULL;
	size_t len;
	struct loopdev_cxt lc;
	int rc = 0, lo_flags = 0;
	uint64_t offset = 0, sizelimit = 0;
	bool reuse = 0;

	backing_file = mnt_fs_get_srcpath(cxt->fs);
	if (!backing_file)
		return -EINVAL;

	if (cxt->mountflags & MS_RDONLY) {
		lo_flags |= LO_FLAGS_READ_ONLY;
	}

	optstr = mnt_fs_get_user_options(cxt->fs);

	/*
	 * loop=
	 */
	if (rc == 0 && (cxt->user_mountflags & MNT_MS_LOOP) &&
	    mnt_optstr_get_option(optstr, "loop", &val, &len) == 0 && val) {
		loopval = strndup(val, len);
		rc = loopval ? 0 : -ENOMEM;
	}

	/*
	 * offset=
	 */
	if (rc == 0 && (cxt->user_mountflags & MNT_MS_OFFSET) &&
	    mnt_optstr_get_option(optstr, "offset", &val, &len) == 0) {
		rc = mnt_parse_offset(val, len, &offset);
		if (rc) {
			rc = -MNT_ERR_MOUNTOPT;
		}
	}

	/*
	 * sizelimit=
	 */
	if (rc == 0 && (cxt->user_mountflags & MNT_MS_SIZELIMIT) &&
	    mnt_optstr_get_option(optstr, "sizelimit", &val, &len) == 0) {
		rc = mnt_parse_offset(val, len, &sizelimit);
		if (rc) {
			rc = -MNT_ERR_MOUNTOPT;
		}
	}

	/*
	 * encryption=
	 */
	if (rc == 0 && (cxt->user_mountflags & MNT_MS_ENCRYPTION) &&
	    mnt_optstr_get_option(optstr, "encryption", &val, &len) == 0) {
		rc = -MNT_ERR_MOUNTOPT;
	}

	if (rc == 0 && is_mounted_same_loopfile(cxt,
				mnt_context_get_target(cxt),
				backing_file, offset))
		rc = -EBUSY;

	if (rc)
		goto done_no_deinit;

	/* It is possible to mount the same file more times. If we set more
	 * than one loop device referring to the same file, kernel has no
	 * mechanism to detect it. To prevent data corruption, the same loop
	 * device has to be recycled.
	*/
	if (backing_file) {
		rc = loopcxt_init(&lc, 0);
		if (rc)
			goto done_no_deinit;

		rc = loopcxt_find_overlap(&lc, backing_file, offset, sizelimit);
		switch (rc) {
		case 0: /* not found */
			loopcxt_deinit(&lc);
			break;

		case 1:	/* overlap */
			rc = -MNT_ERR_LOOPOVERLAP;
			goto done;

		case 2: /* overlap -- full size and offset match (reuse) */
		{
			uint32_t lc_encrypt_type = 0;

			/* Open loop device to block device autoclear... */
			if (loopcxt_get_fd(&lc) < 0) {
				rc = -errno;
				goto done;
			}

			/*
			 * Now that we certainly have the loop device open,
			 * verify the loop device was not autocleared in the
			 * mean time.
			 */
			if (!loopcxt_get_info(&lc)) {
				loopcxt_deinit(&lc);
				break;
			}

			/* Once a loop is initialized RO, there is no
			 * way to change its parameters. */
			if (loopcxt_is_readonly(&lc)
			    && !(lo_flags & LO_FLAGS_READ_ONLY)) {
				rc = -EROFS;
				goto done;
			}

			/* This is no more supported, but check to be safe. */
			if (loopcxt_get_encrypt_type(&lc, &lc_encrypt_type) == 0
			    && lc_encrypt_type != LO_CRYPT_NONE) {
				rc = -MNT_ERR_LOOPOVERLAP;
				goto done;
			}
			rc = 0;
			/* loop= used with argument. Conflict will occur. */
			if (loopval) {
				rc = -MNT_ERR_LOOPOVERLAP;
				goto done;
			} else {
				reuse = 1;
				goto success;
			}
		}
		default: /* error */
			goto done;
		}
	}

	rc = loopcxt_init(&lc, 0);
	if (rc)
		goto done_no_deinit;
	if (loopval) {
		rc = loopcxt_set_device(&lc, loopval);
		if (rc == 0)
			loopdev = loopcxt_get_device(&lc);
	}
	if (rc)
		goto done;

	/* since 2.6.37 we don't have to store backing filename to mtab
	 * because kernel provides the name in /sys.
	 */
	if (get_linux_version() >= KERNEL_VERSION(2, 6, 37) ||
	    !mnt_context_mtab_writable(cxt)) {
		lo_flags |= LO_FLAGS_AUTOCLEAR;
	}

	do {
		/* found free device */
		if (!loopdev) {
			rc = loopcxt_find_unused(&lc);
			if (rc)
				goto done;
		}

		/* set device attributes
		 * -- note that loopcxt_find_unused() resets "lc"
		 */
		rc = loopcxt_set_backing_file(&lc, backing_file);

		if (!rc && offset)
			rc = loopcxt_set_offset(&lc, offset);
		if (!rc && sizelimit)
			rc = loopcxt_set_sizelimit(&lc, sizelimit);
		if (!rc)
			loopcxt_set_flags(&lc, lo_flags);
		if (rc) {
			goto done;
		}

		/* setup the device */
		rc = loopcxt_setup_device(&lc);
		if (!rc)
			break;		/* success */

		if (loopdev || rc != -EBUSY) {
			rc = -MNT_ERR_LOOPDEV;
			goto done;
		}
	} while (1);

success:
	if (!rc)
		rc = mnt_fs_set_source(cxt->fs, loopcxt_get_device(&lc));

	if (!rc) {
		/* success */
		cxt->flags |= MNT_FL_LOOPDEV_READY;

		if (reuse || ( (cxt->user_mountflags & MNT_MS_LOOP) &&
		    loopcxt_is_autoclear(&lc))) {
			/*
			 * autoclear flag accepted by the kernel, don't store
			 * the "loop=" option to mtab.
			 */
			cxt->user_mountflags &= ~MNT_MS_LOOP;
			mnt_optstr_remove_option(&cxt->fs->user_optstr, "loop");
		}

		if (!(cxt->mountflags & MS_RDONLY) &&
		    loopcxt_is_readonly(&lc))
			/*
			 * mount planned read-write, but loopdev is read-only,
			 * let's fix mount options...
			 */
			mnt_context_set_mflags(cxt, cxt->mountflags | MS_RDONLY);

		/* we have to keep the device open until mount(1),
		 * otherwise it will be auto-cleared by kernel
		 */
		cxt->loopdev_fd = loopcxt_get_fd(&lc);
		if (cxt->loopdev_fd < 0) {
			rc = -errno;
		} else
			loopcxt_set_fd(&lc, -1, 0);
	}
done:
	loopcxt_deinit(&lc);
done_no_deinit:
	free(loopval);
	return rc;
}

/*
 * Deletes loop device
 */
int mnt_context_delete_loopdev(struct libmnt_context *cxt)
{
	const char *src;
	int rc;

	src = mnt_fs_get_srcpath(cxt->fs);
	if (!src)
		return -EINVAL;

	if (cxt->loopdev_fd > -1)
		close(cxt->loopdev_fd);

	rc = loopdev_delete(src);
	cxt->flags &= ~MNT_FL_LOOPDEV_READY;
	cxt->loopdev_fd = -1;

	return rc;
}

/*
 * Clears loopdev stuff in context, should be called after
 * failed or successful mount(2).
 */
int mnt_context_clear_loopdev(struct libmnt_context *cxt)
{
	if (mnt_context_get_status(cxt) == 0 &&
	    (cxt->flags & MNT_FL_LOOPDEV_READY)) {
		/*
		 * mount(2) failed, delete loopdev
		 */
		mnt_context_delete_loopdev(cxt);

	} else if (cxt->loopdev_fd > -1) {
		/*
		 * mount(2) success, close the device
		 */
		close(cxt->loopdev_fd);
	}
	cxt->loopdev_fd = -1;
	return 0;
}

