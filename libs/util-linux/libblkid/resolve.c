/*
 * resolve.c - resolve names and tags into specific devices
 *
 * Copyright (C) 2001, 2003 Theodore Ts'o.
 * Copyright (C) 2001 Andreas Dilger
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 * %End-Header%
 */

#include <stdlib.h>
#include <string.h>
#include "blkidP.h"

/*
 * Find a tagname (e.g. LABEL or UUID) on a specific device.
 */
char *blkid_get_tag_value(blkid_cache cache, const char *tagname,
			  const char *devname)
{
	blkid_tag found;
	blkid_dev dev;
	blkid_cache c = cache;
	char *ret = NULL;

	if (!devname)
		return NULL;
	if (!cache && blkid_get_cache(&c, NULL) < 0)
		return NULL;

	if ((dev = blkid_get_dev(c, devname, BLKID_DEV_NORMAL)) &&
	    (found = blkid_find_tag_dev(dev, tagname)))
		ret = found->bit_val ? strdup(found->bit_val) : NULL;

	if (!cache)
		blkid_put_cache(c);

	return ret;
}

/*
 * Locate a device name from a token (NAME=value string), or (name, value)
 * pair.  In the case of a token, value is ignored.  If the "token" is not
 * of the form "NAME=value" and there is no value given, then it is assumed
 * to be the actual devname and a copy is returned.
 */
char *blkid_get_devname(blkid_cache cache, const char *token,
			const char *value)
{
	blkid_dev dev;
	blkid_cache c = cache;
	char *t = NULL, *v = NULL;
	char *ret = NULL;

	if (!token)
		return NULL;
	if (!cache && blkid_get_cache(&c, NULL) < 0)
		return NULL;

	if (!value) {
		if (!strchr(token, '=')) {
			ret = strdup(token);
			goto out;
		}
		if (blkid_parse_tag_string(token, &t, &v) != 0 || !t || !v)
			goto out;
		token = t;
		value = v;
	}

	dev = blkid_find_dev_with_tag(c, token, value);
	if (!dev)
		goto out;

	ret = dev->bid_name ? strdup(dev->bid_name) : NULL;
out:
	free(t);
	free(v);
	if (!cache)
		blkid_put_cache(c);
	return ret;
}
