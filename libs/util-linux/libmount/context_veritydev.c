/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libmount from util-linux project.
 *
 * Copyright (C) 2019 Microsoft Corporation
 *
 * libmount is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "mountP.h"

int mnt_context_setup_veritydev(struct libmnt_context *cxt __attribute__ ((__unused__)))
{
	return 0;
}

int mnt_context_deferred_delete_veritydev(struct libmnt_context *cxt __attribute__ ((__unused__)))
{
	return 0;
}

int mnt_context_is_veritydev(struct libmnt_context *cxt)
{
	const char *src;

	if (!cxt->fs)
		return 0;
	src = mnt_fs_get_srcpath(cxt->fs);
	if (!src)
		return 0;		/* backing file not set */

	if (cxt->user_mountflags & (MNT_MS_HASH_DEVICE |
				    MNT_MS_ROOT_HASH |
				    MNT_MS_HASH_OFFSET)) {
		return -ENOTSUP;
	}

	if (!strncmp(src, "/dev/mapper/libmnt_", strlen("/dev/mapper/libmnt_"))) {
		return -ENOTSUP;
	}

	return 0;
}
