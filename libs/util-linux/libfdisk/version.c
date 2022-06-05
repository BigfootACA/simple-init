/*
 * version.c - Return the version of the library
 *
 * Copyright (C) 2015 Karel Zak <kzak@redhat.com>
 *
 */

/**
 * SECTION: version-utils
 * @title: Version functions
 * @short_description: functions to get the library version.
 */

#include <ctype.h>

#include "fdiskP.h"

static const char *lib_version = LIBFDISK_VERSION;
static const char *lib_features[] = {
	"blkid",
	NULL
};

/**
 * fdisk_parse_version_string:
 * @ver_string: version string (e.g "2.18.0")
 *
 * Returns: release version code.
 */
int fdisk_parse_version_string(const char *ver_string)
{
	const char *cp;
	int version = 0;

	for (cp = ver_string; *cp; cp++) {
		if (*cp == '.')
			continue;
		if (!isdigit(*cp))
			break;
		version = (version * 10) + (*cp - '0');
	}
	return version;
}

/**
 * fdisk_get_library_version:
 * @ver_string: return pointer to the static library version string if not NULL
 *
 * Returns: release version number.
 */
int fdisk_get_library_version(const char **ver_string)
{
	if (ver_string)
		*ver_string = lib_version;

	return fdisk_parse_version_string(lib_version);
}

/**
 * fdisk_get_library_features:
 * @features: returns a pointer to the static array of strings, the array is
 *            terminated by NULL.
 *
 * Returns: number of items in the features array not including the last NULL,
 *          or less than zero in case of error
 *
 * Example:
 * <informalexample>
 *   <programlisting>
 *	const char *features;
 *
 *	fdisk_get_library_features(&features);
 *	while (features && *features)
 *		printf("%s\n", *features++);
 *   </programlisting>
 * </informalexample>
 *
 */
int fdisk_get_library_features(const char ***features)
{
	if (!features)
		return -EINVAL;

	*features = lib_features;
	return ARRLEN(lib_features) - 1;
}
