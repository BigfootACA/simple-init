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

/**
 * SECTION: update
 * @title: Tables update
 * @short_description: userspace mount information management
 *
 * The struct libmnt_update provides an abstraction to manage mount options in
 * userspace independently of system configuration. This low-level API works on
 * systems both with and without /etc/mtab. On systems without the regular /etc/mtab
 * file, the userspace mount options (e.g. user=) are stored in the /run/mount/utab
 * file.
 *
 * It's recommended to use high-level struct libmnt_context API.
 */

#include <fcntl.h>
#include <signal.h>
#include "mountP.h"
#include "mangle.h"
#include "pathnames.h"

struct libmnt_update {
	char		*target;
	struct libmnt_fs *fs;
	char		*filename;
	unsigned long	mountflags;
	int		userspace_only;
	int		ready;

	struct libmnt_table *mountinfo;
};

static int set_fs_root(struct libmnt_update *upd, struct libmnt_fs *fs, unsigned long mountflags);
static int utab_new_entry(struct libmnt_update *upd, struct libmnt_fs *fs, unsigned long mountflags);

/**
 * mnt_new_update:
 *
 * Returns: newly allocated update handler
 */
struct libmnt_update *mnt_new_update(void)
{
	struct libmnt_update *upd;

	upd = calloc(1, sizeof(*upd));
	if (!upd)
		return NULL;

	return upd;
}

/**
 * mnt_free_update:
 * @upd: update
 *
 * Deallocates struct libmnt_update handler.
 */
void mnt_free_update(struct libmnt_update *upd)
{
	if (!upd)
		return;

	mnt_unref_fs(upd->fs);
	mnt_unref_table(upd->mountinfo);
	free(upd->target);
	free(upd->filename);
	free(upd);
}

/*
 * Returns 0 on success, <0 in case of error.
 */
int mnt_update_set_filename(struct libmnt_update *upd, const char *filename,
			    int userspace_only)
{
	const char *path = NULL;
	int rw = 0;

	if (!upd)
		return -EINVAL;

	/* filename explicitly defined */
	if (filename) {
		char *p = strdup(filename);
		if (!p)
			return -ENOMEM;

		upd->userspace_only = userspace_only;
		free(upd->filename);
		upd->filename = p;
	}

	if (upd->filename)
		return 0;

	/* detect tab filename -- /etc/mtab or /run/mount/utab
	 */
#ifdef USE_LIBMOUNT_SUPPORT_MTAB
	mnt_has_regular_mtab(&path, &rw);
#endif
	if (!rw) {
		path = NULL;
		mnt_has_regular_utab(&path, &rw);
		if (!rw)
			return -EACCES;
		upd->userspace_only = 1;
	}
	upd->filename = strdup(path);
	if (!upd->filename)
		return -ENOMEM;

	return 0;
}

/**
 * mnt_update_get_filename:
 * @upd: update
 *
 * This function returns the file name (e.g. /etc/mtab) of the up-dated file.
 *
 * Returns: pointer to filename that will be updated or NULL in case of error.
 */
const char *mnt_update_get_filename(struct libmnt_update *upd)
{
	return upd ? upd->filename : NULL;
}

/**
 * mnt_update_is_ready:
 * @upd: update handler
 *
 * Returns: 1 if entry described by @upd is successfully prepared and will be
 * written to the mtab/utab file.
 */
int mnt_update_is_ready(struct libmnt_update *upd)
{
	return upd ? upd->ready : 0;
}

/**
 * mnt_update_set_fs:
 * @upd: update handler
 * @mountflags: MS_* flags
 * @target: umount target, must be NULL for mount
 * @fs: mount filesystem description, must be NULL for umount
 *
 * Returns: <0 in case on error, 0 on success, 1 if update is unnecessary.
 */
int mnt_update_set_fs(struct libmnt_update *upd, unsigned long mountflags,
		      const char *target, struct libmnt_fs *fs)
{
	int rc;

	if (!upd)
		return -EINVAL;
	if ((mountflags & MS_MOVE) && (!fs || !mnt_fs_get_srcpath(fs)))
		return -EINVAL;
	if (target && fs)
		return -EINVAL;

	mnt_unref_fs(upd->fs);
	free(upd->target);
	upd->ready = 0;
	upd->fs = NULL;
	upd->target = NULL;
	upd->mountflags = 0;

	if (mountflags & MS_PROPAGATION)
		return 1;

	upd->mountflags = mountflags;

	rc = mnt_update_set_filename(upd, NULL, 0);
	if (rc) {
		return rc;	/* error or no file available (rc = 1) */
	}
	if (target) {
		upd->target = strdup(target);
		if (!upd->target)
			return -ENOMEM;

	} else if (fs) {
		if (upd->userspace_only && !(mountflags & MS_MOVE)) {
			rc = utab_new_entry(upd, fs, mountflags);
			if (rc)
				return rc;
		} else {
			upd->fs = mnt_copy_mtab_fs(fs);
			if (!upd->fs)
				return -ENOMEM;

		}
	}

	upd->ready = 1;
	return 0;
}

/**
 * mnt_update_get_fs:
 * @upd: update
 *
 * Returns: update filesystem entry or NULL
 */
struct libmnt_fs *mnt_update_get_fs(struct libmnt_update *upd)
{
	return upd ? upd->fs : NULL;
}

/**
 * mnt_update_get_mflags:
 * @upd: update
 *
 * Returns: mount flags as was set by mnt_update_set_fs()
 */
unsigned long mnt_update_get_mflags(struct libmnt_update *upd)
{
	return upd ? upd->mountflags : 0;
}

/**
 * mnt_update_force_rdonly:
 * @upd: update
 * @rdonly: is read-only?
 *
 * Returns: 0 on success and negative number in case of error.
 */
int mnt_update_force_rdonly(struct libmnt_update *upd, int rdonly)
{
	int rc = 0;

	if (!upd || !upd->fs)
		return -EINVAL;

	if (rdonly && (upd->mountflags & MS_RDONLY))
		return 0;
	if (!rdonly && !(upd->mountflags & MS_RDONLY))
		return 0;

	if (!upd->userspace_only) {
		/* /etc/mtab -- we care about VFS options there */
		const char *o = mnt_fs_get_options(upd->fs);
		char *n = o ? strdup(o) : NULL;

		if (n)
			mnt_optstr_remove_option(&n, rdonly ? "rw" : "ro");
		if (!mnt_optstr_prepend_option(&n, rdonly ? "ro" : "rw", NULL))
			rc = mnt_fs_set_options(upd->fs, n);

		free(n);
	}

	if (rdonly)
		upd->mountflags &= ~MS_RDONLY;
	else
		upd->mountflags |= MS_RDONLY;

	return rc;
}


/*
 * Allocates an utab entry (upd->fs) for mount/remount. This function should be
 * called *before* mount(2) syscall. The @fs is used as a read-only template.
 *
 * Returns: 0 on success, negative number on error, 1 if utab's update is
 *          unnecessary.
 */
static int utab_new_entry(struct libmnt_update *upd, struct libmnt_fs *fs,
			  unsigned long mountflags)
{
	int rc = 0;
	const char *o, *a;
	char *u = NULL;

	o = mnt_fs_get_user_options(fs);
	a = mnt_fs_get_attributes(fs);
	upd->fs = NULL;

	if (o) {
		/* remove non-mtab options */
		rc = mnt_optstr_get_options(o, &u,
				mnt_get_builtin_optmap(MNT_USERSPACE_MAP),
				MNT_NOMTAB);
		if (rc)
			goto err;
	}

	if (!u && !a) {
		return 1;
	}

	/* allocate the entry */
	upd->fs = mnt_copy_fs(NULL, fs);
	if (!upd->fs) {
		rc = -ENOMEM;
		goto err;
	}

	rc = mnt_fs_set_options(upd->fs, u);
	if (rc)
		goto err;
	rc = mnt_fs_set_attributes(upd->fs, a);
	if (rc)
		goto err;

	if (!(mountflags & MS_REMOUNT)) {
		rc = set_fs_root(upd, fs, mountflags);
		if (rc)
			goto err;
	}

	free(u);
	return 0;
err:
	free(u);
	mnt_unref_fs(upd->fs);
	upd->fs = NULL;
	return rc;
}

/*
 * Sets fs-root and fs-type to @upd->fs according to the @fs template and
 * @mountfalgs. For MS_BIND mountflag it reads information about the source
 * filesystem from /proc/self/mountinfo.
 */
static int set_fs_root(struct libmnt_update *upd, struct libmnt_fs *fs,
		       unsigned long mountflags)
{
	struct libmnt_fs *src_fs;
	char *fsroot = NULL;
	const char *src, *fstype;
	int rc = 0;

	fstype = mnt_fs_get_fstype(fs);

	if (mountflags & MS_BIND) {
		if (!upd->mountinfo)
			upd->mountinfo = mnt_new_table_from_file(_PATH_PROC_MOUNTINFO);
		src = mnt_fs_get_srcpath(fs);
		if (src) {
			 rc = mnt_fs_set_bindsrc(upd->fs, src);
			 if (rc)
				 goto err;
		}

	} else if (fstype && (strcmp(fstype, "btrfs") == 0 || strcmp(fstype, "auto") == 0)) {
		if (!upd->mountinfo)
			upd->mountinfo = mnt_new_table_from_file(_PATH_PROC_MOUNTINFO);
	}

	src_fs = mnt_table_get_fs_root(upd->mountinfo, fs,
					mountflags, &fsroot);
	if (src_fs) {
		src = mnt_fs_get_srcpath(src_fs);
		rc = mnt_fs_set_source(upd->fs, src);
		if (rc)
			goto err;

		mnt_fs_set_fstype(upd->fs, mnt_fs_get_fstype(src_fs));
	}

	upd->fs->root = fsroot;
	return 0;
err:
	free(fsroot);
	return rc;
}

/* mtab and fstab update -- returns zero on success
 */
static int fprintf_mtab_fs(FILE *f, struct libmnt_fs *fs)
{
	const char *o, *src, *fstype, *comm;
	char *m1, *m2, *m3, *m4;
	int rc;

	comm = mnt_fs_get_comment(fs);
	src = mnt_fs_get_source(fs);
	fstype = mnt_fs_get_fstype(fs);
	o = mnt_fs_get_options(fs);

	m1 = src ? mangle(src) : "none";
	m2 = mangle(mnt_fs_get_target(fs));
	m3 = fstype ? mangle(fstype) : "none";
	m4 = o ? mangle(o) : "rw";

	if (m1 && m2 && m3 && m4) {
		if (comm)
			fputs(comm, f);
		rc = fprintf(f, "%s %s %s %s %d %d\n",
				m1, m2, m3, m4,
				mnt_fs_get_freq(fs),
				mnt_fs_get_passno(fs));
		if (rc > 0)
			rc = 0;
	} else
		rc = -ENOMEM;

	if (src)
		free(m1);
	free(m2);
	if (fstype)
		free(m3);
	if (o)
		free(m4);

	return rc;
}

static int fprintf_utab_fs(FILE *f, struct libmnt_fs *fs)
{
	char *p;
	int rc = 0;

	if (!fs || !f)
		return -EINVAL;

	p = mangle(mnt_fs_get_source(fs));
	if (p) {
		rc = fprintf(f, "SRC=%s ", p);
		free(p);
	}
	if (rc >= 0) {
		p = mangle(mnt_fs_get_target(fs));
		if (p) {
			rc = fprintf(f, "TARGET=%s ", p);
			free(p);
		}
	}
	if (rc >= 0) {
		p = mangle(mnt_fs_get_root(fs));
		if (p) {
			rc = fprintf(f, "ROOT=%s ", p);
			free(p);
		}
	}
	if (rc >= 0) {
		p = mangle(mnt_fs_get_bindsrc(fs));
		if (p) {
			rc = fprintf(f, "BINDSRC=%s ", p);
			free(p);
		}
	}
	if (rc >= 0) {
		p = mangle(mnt_fs_get_attributes(fs));
		if (p) {
			rc = fprintf(f, "ATTRS=%s ", p);
			free(p);
		}
	}
	if (rc >= 0) {
		p = mangle(mnt_fs_get_user_options(fs));
		if (p) {
			rc = fprintf(f, "OPTS=%s", p);
			free(p);
		}
	}
	if (rc >= 0)
		rc = fprintf(f, "\n");

	if (rc > 0)
		rc = 0;	/* success */
	return rc;
}

static int update_table(struct libmnt_update *upd, struct libmnt_table *tb)
{
	FILE *f;
	int rc, fd;
	char *uq = NULL;

	if (!tb || !upd->filename)
		return -EINVAL;

	fd = mnt_open_uniq_filename(upd->filename, &uq);
	if (fd < 0)
		return fd;	/* error */

	f = fdopen(fd, "we");
	if (f) {
		struct stat st;
		struct libmnt_iter itr;
		struct libmnt_fs *fs;

		mnt_reset_iter(&itr, MNT_ITER_FORWARD);

		if (tb->comms && mnt_table_get_intro_comment(tb))
			fputs(mnt_table_get_intro_comment(tb), f);

		while(mnt_table_next_fs(tb, &itr, &fs) == 0) {
			if (upd->userspace_only)
				rc = fprintf_utab_fs(f, fs);
			else
				rc = fprintf_mtab_fs(f, fs);
			if (rc) {
				goto leave;
			}
		}
		if (tb->comms && mnt_table_get_trailing_comment(tb))
			fputs(mnt_table_get_trailing_comment(tb), f);

		if (fflush(f) != 0) {
			rc = -errno;
			goto leave;
		}

		rc = fchmod(fd, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) ? -errno : 0;

		if (!rc && stat(upd->filename, &st) == 0)
			/* Copy uid/gid from the present file before renaming. */
			rc = fchown(fd, st.st_uid, st.st_gid) ? -errno : 0;

		fclose(f);
		f = NULL;

		if (!rc)
			rc = rename(uq, upd->filename) ? -errno : 0;
	} else {
		rc = -errno;
		close(fd);
	}

leave:
	if (f)
		fclose(f);

	unlink(uq);	/* be paranoid */
	free(uq);
	return rc;
}

/**
 * mnt_table_write_file
 * @tb: parsed file (e.g. fstab)
 * @file: target
 *
 * This function writes @tb to @file.
 *
 * Returns: 0 on success, negative number on error.
 */
int mnt_table_write_file(struct libmnt_table *tb, FILE *file)
{
	int rc = 0;
	struct libmnt_iter itr;
	struct libmnt_fs *fs;

	if (tb->comms && mnt_table_get_intro_comment(tb))
		fputs(mnt_table_get_intro_comment(tb), file);

	mnt_reset_iter(&itr, MNT_ITER_FORWARD);
	while(mnt_table_next_fs(tb, &itr, &fs) == 0) {
		rc = fprintf_mtab_fs(file, fs);
		if (rc)
			return rc;
	}
	if (tb->comms && mnt_table_get_trailing_comment(tb))
		fputs(mnt_table_get_trailing_comment(tb), file);

	if (fflush(file) != 0)
		rc = -errno;

	return rc;
}

/**
 * mnt_table_replace_file
 * @tb: parsed file (e.g. fstab)
 * @filename: target
 *
 * This function replaces @file by the new content from @tb.
 *
 * Returns: 0 on success, negative number on error.
 */
int mnt_table_replace_file(struct libmnt_table *tb, const char *filename)
{
	int fd, rc = 0;
	FILE *f;
	char *uq = NULL;

	fd = mnt_open_uniq_filename(filename, &uq);
	if (fd < 0)
		return fd;	/* error */

	f = fdopen(fd, "we");
	if (f) {
		struct stat st;

		mnt_table_write_file(tb, f);

		if (fflush(f) != 0) {
			rc = -errno;
			goto leave;
		}

		rc = fchmod(fd, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) ? -errno : 0;

		if (!rc && stat(filename, &st) == 0)
			/* Copy uid/gid from the present file before renaming. */
			rc = fchown(fd, st.st_uid, st.st_gid) ? -errno : 0;

		fclose(f);
		f = NULL;

		if (!rc)
			rc = rename(uq, filename) ? -errno : 0;
	} else {
		rc = -errno;
		close(fd);
	}

leave:
	if (f)
		fclose(f);
	unlink(uq);
	free(uq);
	return rc;
}

static int add_file_entry(struct libmnt_table *tb, struct libmnt_update *upd)
{
	struct libmnt_fs *fs;

	fs = mnt_copy_fs(NULL, upd->fs);
	if (!fs)
		return -ENOMEM;

	mnt_table_add_fs(tb, fs);
	mnt_unref_fs(fs);

	return update_table(upd, tb);
}

static int update_add_entry(struct libmnt_update *upd, struct libmnt_lock *lc)
{
	struct libmnt_table *tb;
	int rc = 0;

	if (lc)
		rc = mnt_lock_file(lc);
	if (rc)
		return -MNT_ERR_LOCK;

	tb = __mnt_new_table_from_file(upd->filename,
			upd->userspace_only ? MNT_FMT_UTAB : MNT_FMT_MTAB, 1);
	if (tb)
		rc = add_file_entry(tb, upd);
	if (lc)
		mnt_unlock_file(lc);

	mnt_unref_table(tb);
	return rc;
}

static int update_remove_entry(struct libmnt_update *upd, struct libmnt_lock *lc)
{
	struct libmnt_table *tb;
	int rc = 0;

	if (lc)
		rc = mnt_lock_file(lc);
	if (rc)
		return -MNT_ERR_LOCK;

	tb = __mnt_new_table_from_file(upd->filename,
			upd->userspace_only ? MNT_FMT_UTAB : MNT_FMT_MTAB, 1);
	if (tb) {
		struct libmnt_fs *rem = mnt_table_find_target(tb, upd->target, MNT_ITER_BACKWARD);
		if (rem) {
			mnt_table_remove_fs(tb, rem);
			rc = update_table(upd, tb);
		}
	}
	if (lc)
		mnt_unlock_file(lc);

	mnt_unref_table(tb);
	return rc;
}

static int update_modify_target(struct libmnt_update *upd, struct libmnt_lock *lc)
{
	struct libmnt_table *tb = NULL;
	int rc = 0;

	if (lc)
		rc = mnt_lock_file(lc);
	if (rc)
		return -MNT_ERR_LOCK;

	tb = __mnt_new_table_from_file(upd->filename,
			upd->userspace_only ? MNT_FMT_UTAB : MNT_FMT_MTAB, 1);
	if (tb) {
		struct libmnt_fs *cur = mnt_table_find_target(tb,
				mnt_fs_get_srcpath(upd->fs), MNT_ITER_BACKWARD);
		if (cur) {
			rc = mnt_fs_set_target(cur, mnt_fs_get_target(upd->fs));
			if (!rc)
				rc = update_table(upd, tb);
		}
	}

	if (lc)
		mnt_unlock_file(lc);

	mnt_unref_table(tb);
	return rc;
}

static int update_modify_options(struct libmnt_update *upd, struct libmnt_lock *lc)
{
	struct libmnt_table *tb = NULL;
	int rc = 0;
	struct libmnt_fs *fs;

	fs = upd->fs;

	if (lc)
		rc = mnt_lock_file(lc);
	if (rc)
		return -MNT_ERR_LOCK;

	tb = __mnt_new_table_from_file(upd->filename,
			upd->userspace_only ? MNT_FMT_UTAB : MNT_FMT_MTAB, 1);
	if (tb) {
		struct libmnt_fs *cur = mnt_table_find_target(tb,
					mnt_fs_get_target(fs),
					MNT_ITER_BACKWARD);
		if (cur) {
			if (upd->userspace_only)
				rc = mnt_fs_set_attributes(cur,	mnt_fs_get_attributes(fs));
			if (!rc)
				rc = mnt_fs_set_options(cur, mnt_fs_get_options(fs));
			if (!rc)
				rc = update_table(upd, tb);
		} else
			rc = add_file_entry(tb, upd);	/* not found, add new */
	}

	if (lc)
		mnt_unlock_file(lc);

	mnt_unref_table(tb);
	return rc;
}

/**
 * mnt_update_table:
 * @upd: update
 * @lc: lock or NULL
 *
 * High-level API to update /etc/mtab (or private /run/mount/utab file).
 *
 * The @lc lock is optional and will be created if necessary. Note that
 * an automatically created lock blocks all signals.
 *
 * See also mnt_lock_block_signals() and mnt_context_get_lock().
 *
 * Returns: 0 on success, negative number on error.
 */
int mnt_update_table(struct libmnt_update *upd, struct libmnt_lock *lc)
{
	struct libmnt_lock *lc0 = lc;
	int rc = -EINVAL;

	if (!upd || !upd->filename)
		return -EINVAL;
	if (!upd->ready)
		return 0;

	if (!lc) {
		lc = mnt_new_lock(upd->filename, 0);
		if (lc)
			mnt_lock_block_signals(lc, 1);
	}
	if (lc && upd->userspace_only)
		mnt_lock_use_simplelock(lc, 1);	/* use flock */

	if (!upd->fs && upd->target)
		rc = update_remove_entry(upd, lc);	/* umount */
	else if (upd->mountflags & MS_MOVE)
		rc = update_modify_target(upd, lc);	/* move */
	else if (upd->mountflags & MS_REMOUNT)
		rc = update_modify_options(upd, lc);	/* remount */
	else if (upd->fs)
		rc = update_add_entry(upd, lc);	/* mount */

	upd->ready = 0;
	if (lc != lc0)
		 mnt_free_lock(lc);
	return rc;
}

int mnt_update_already_done(struct libmnt_update *upd, struct libmnt_lock *lc)
{
	struct libmnt_table *tb = NULL;
	struct libmnt_lock *lc0 = lc;
	int rc = 0;

	if (!upd || !upd->filename || (!upd->fs && !upd->target))
		return -EINVAL;

	if (!lc) {
		lc = mnt_new_lock(upd->filename, 0);
		if (lc)
			mnt_lock_block_signals(lc, 1);
	}
	if (lc && upd->userspace_only)
		mnt_lock_use_simplelock(lc, 1);	/* use flock */
	if (lc) {
		rc = mnt_lock_file(lc);
		if (rc) {
			rc = -MNT_ERR_LOCK;
			goto done;
		}
	}

	tb = __mnt_new_table_from_file(upd->filename,
			upd->userspace_only ? MNT_FMT_UTAB : MNT_FMT_MTAB, 1);
	if (lc)
		mnt_unlock_file(lc);
	if (!tb)
		goto done;

	if (upd->fs) {
		/* mount */
		const char *tgt = mnt_fs_get_target(upd->fs);
		const char *src = mnt_fs_get_bindsrc(upd->fs) ?
					mnt_fs_get_bindsrc(upd->fs) :
					mnt_fs_get_source(upd->fs);

		if (mnt_table_find_pair(tb, src, tgt, MNT_ITER_BACKWARD)) {
			rc = 1;
		}
	} else if (upd->target) {
		/* umount */
		if (!mnt_table_find_target(tb, upd->target, MNT_ITER_BACKWARD)) {
			rc = 1;
		}
	}

	mnt_unref_table(tb);
done:
	if (lc && lc != lc0)
		mnt_free_lock(lc);
	return rc;
}
