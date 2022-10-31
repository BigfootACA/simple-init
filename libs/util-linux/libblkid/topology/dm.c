/*
 * device-mapper (dm) topology
 * -- this is fallback for old systems where the topology information is not
 *    exported by sysfs
 *
 * Copyright (C) 2009 Karel Zak <kzak@redhat.com>
 *
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include "topology.h"

static int is_dm_device(dev_t devno)
{
	return blkid_driver_has_major("device-mapper", major(devno));
}

static int probe_dm_tp(blkid_probe pr,
		const struct blkid_idmag *mag __attribute__((__unused__)))
{
	const char *paths[] = {
		"/usr/local/sbin/dmsetup",
		"/usr/local/bin/dmsetup",
		"/usr/sbin/dmsetup",
		"/usr/bin/dmsetup",
		"/sbin/dmsetup",
		"/bin/dmsetup"
	};
	int dmpipe[] = { -1, -1 }, stripes, stripesize;
	const char *cmd = NULL;
	FILE *stream = NULL;
	long long  offset, size;
	size_t i;
	dev_t devno = blkid_probe_get_devno(pr);

	if (!devno)
		goto nothing;		/* probably not a block device */
	if (!is_dm_device(devno))
		goto nothing;

	for (i = 0; i < ARRLEN(paths); i++) {
		struct stat sb;
		if (stat(paths[i], &sb) == 0) {
			cmd = paths[i];
			break;
		}
	}

	if (!cmd)
		goto nothing;
	if (pipe(dmpipe) < 0) {
		goto nothing;
	}

	switch (fork()) {
	case 0:
	{
		const char *dmargv[7];
	        char maj[16], min[16];

		/* Plumbing */
		close(dmpipe[0]);

		if (dmpipe[1] != STDOUT_FILENO)
			dup2(dmpipe[1], STDOUT_FILENO);

		if (setgid(getgid()) < 0)
			exit(1);

		if (setuid(getuid()) < 0)
			exit(1);

		snprintf(maj, sizeof(maj), "%d", major(devno));
		snprintf(min, sizeof(min), "%d", minor(devno));

		dmargv[0] = cmd;
		dmargv[1] = "table";
		dmargv[2] = "-j";
		dmargv[3] = maj;
		dmargv[4] = "-m";
		dmargv[5] = min;
		dmargv[6] = NULL;

		execv(dmargv[0], (char * const *) dmargv);

		exit(1);
	}
	case -1:
		goto nothing;
	default:
		break;
	}

	stream = fdopen(dmpipe[0], "re");
	if (!stream)
		goto nothing;

	if (fscanf(stream, "%lld %lld striped %d %d ",
			&offset, &size, &stripes, &stripesize) != 0)
		goto nothing;

	blkid_topology_set_minimum_io_size(pr, stripesize << 9);
	blkid_topology_set_optimal_io_size(pr, (stripes * stripesize) << 9);

	fclose(stream);
	close(dmpipe[1]);
	return 0;

nothing:
	if (stream)
		fclose(stream);
	else if (dmpipe[0] != -1)
		close(dmpipe[0]);
	if (dmpipe[1] != -1)
		close(dmpipe[1]);
	return 1;
}

const struct blkid_idinfo dm_tp_idinfo =
{
	.name		= "dm",
	.probefunc	= probe_dm_tp,
	.magics		= BLKID_NONE_MAGIC
};

