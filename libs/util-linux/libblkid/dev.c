/*
 * dev.c - allocation/initialization/free routines for dev
 *
 * Copyright (C) 2001 Andreas Dilger
 * Copyright (C) 2003 Theodore Ts'o
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 * %End-Header%
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "blkidP.h"

/*
 * NOTE: reference manual is not structured as code. The following section is a generic
 * section for all high-level cache search+iterate routines.
 */

/**
 * SECTION:search
 * @title: Search and iterate
 * @short_description: search devices and iterate over devices in the cache.
 *
 * Note that high-level probing API provides information about superblocks
 * (filesystems/raids) only.  For partitions and topology is necessary to use
 * the low-level API.
 */

blkid_dev blkid_new_dev(void)
{
	blkid_dev dev;

	if (!(dev = calloc(1, sizeof(struct blkid_struct_dev))))
		return NULL;

	INIT_LIST_HEAD(&dev->bid_devs);
	INIT_LIST_HEAD(&dev->bid_tags);

	return dev;
}

void blkid_free_dev(blkid_dev dev)
{
	if (!dev)
		return;

	list_del(&dev->bid_devs);
	while (!list_empty(&dev->bid_tags)) {
		blkid_tag tag = list_entry(dev->bid_tags.next,
					   struct blkid_struct_tag,
					   bit_tags);
		blkid_free_tag(tag);
	}
	free(dev->bid_xname);
	free(dev->bid_name);
	free(dev);
}

/*
 * Given a blkid device, return its name. The function returns the name
 * previously used for blkid_get_dev(). This name does not have to be canonical
 * (real path) name, but for example symlink.
 */
const char *blkid_dev_devname(blkid_dev dev)
{
	if (!dev)
		return NULL;
	if (dev->bid_xname)
		return dev->bid_xname;
	return dev->bid_name;
}

/*
 * dev iteration routines for the public libblkid interface.
 *
 * These routines do not expose the list.h implementation, which are a
 * contamination of the namespace, and which force us to reveal far, far
 * too much of our internal implementation.  I'm not convinced I want
 * to keep list.h in the long term, anyway.  It's fine for kernel
 * programming, but performance is not the #1 priority for this
 * library, and I really don't like the trade-off of type-safety for
 * performance for this application.  [tytso:20030125.2007EST]
 */

/*
 * This series of functions iterate over all devices in a blkid cache
 */
#define DEV_ITERATE_MAGIC	0x01a5284c

struct blkid_struct_dev_iterate {
	int			magic;
	blkid_cache		cache;
	char			*search_type;
	char			*search_value;
	struct list_head	*p;
};

blkid_dev_iterate blkid_dev_iterate_begin(blkid_cache cache)
{
	blkid_dev_iterate iter;

	if (!cache) {
		errno = EINVAL;
		return NULL;
	}

	iter = malloc(sizeof(struct blkid_struct_dev_iterate));
	if (iter) {
		iter->magic = DEV_ITERATE_MAGIC;
		iter->cache = cache;
		iter->p	= cache->bic_devs.next;
		iter->search_type = NULL;
		iter->search_value = NULL;
	}
	return iter;
}

int blkid_dev_set_search(blkid_dev_iterate iter,
				 const char *search_type, const char *search_value)
{
	char *new_type, *new_value;

	if (!iter || iter->magic != DEV_ITERATE_MAGIC || !search_type ||
	    !search_value)
		return -1;
	new_type = malloc(strlen(search_type)+1);
	new_value = malloc(strlen(search_value)+1);
	if (!new_type || !new_value) {
		free(new_type);
		free(new_value);
		return -1;
	}
	strcpy(new_type, search_type);
	strcpy(new_value, search_value);
	free(iter->search_type);
	free(iter->search_value);
	iter->search_type = new_type;
	iter->search_value = new_value;
	return 0;
}

/*
 * Return 0 on success, -1 on error
 */
int blkid_dev_next(blkid_dev_iterate iter,
			  blkid_dev *ret_dev)
{
	blkid_dev		dev;

	if  (!ret_dev || !iter || iter->magic != DEV_ITERATE_MAGIC)
		return -1;
	*ret_dev = NULL;
	while (iter->p != &iter->cache->bic_devs) {
		dev = list_entry(iter->p, struct blkid_struct_dev, bid_devs);
		iter->p = iter->p->next;
		if (iter->search_type &&
		    !blkid_dev_has_tag(dev, iter->search_type,
				       iter->search_value))
			continue;
		*ret_dev = dev;
		return 0;
	}
	return -1;
}

void blkid_dev_iterate_end(blkid_dev_iterate iter)
{
	if (!iter || iter->magic != DEV_ITERATE_MAGIC)
		return;
	iter->magic = 0;
	free(iter->search_type);
	free(iter->search_value);
	free(iter);
}
