/*
 * gen_uuid.c --- generate a DCE-compatible uuid
 *
 * Copyright (C) 1996, 1997, 1998, 1999 Theodore Ts'o.
 *
 * %Begin-Header%
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * %End-Header%
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <net/if.h>
#include <netinet/in.h>

#include "uuidP.h"
#include "uuidd.h"
#include "str.h"

#define THREAD_LOCAL static __thread

/*
 * Get the ethernet hardware address, if we can find it...
 *
 * XXX for a windows version, probably should use GetAdaptersInfo:
 * http://www.codeguru.com/cpp/i-n/network/networkinformation/article.php/c5451
 * commenting out get_node_id just to get gen_uuid to compile under windows
 * is not the right way to go!
 */
static int get_node_id(unsigned char *node_id)
{
	int		sd;
	struct ifreq	ifr, *ifrp;
	struct ifconf	ifc;
	char buf[1024];
	int		n, i;
	unsigned char	*a = NULL;

	sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sd < 0) {
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl (sd, SIOCGIFCONF, (char *)&ifc) < 0) {
		close(sd);
		return -1;
	}
	n = ifc.ifc_len;
	for (i = 0; i < n; i+= sizeof(struct ifreq) ) {
		ifrp = (struct ifreq *)((char *) ifc.ifc_buf+i);
		strncpy(ifr.ifr_name, ifrp->ifr_name, IFNAMSIZ);
#ifdef SIOCGIFHWADDR
		if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0)
			continue;
		a = (unsigned char *) &ifr.ifr_hwaddr.sa_data;
#else
#ifdef SIOCGENADDR
		if (ioctl(sd, SIOCGENADDR, &ifr) < 0)
			continue;
		a = (unsigned char *) ifr.ifr_enaddr;
#else
		close(sd);
		return 0;
#endif /* SIOCGENADDR */
#endif /* SIOCGIFHWADDR */
		if (a == NULL || (!a[0] && !a[1] && !a[2] && !a[3] && !a[4] && !a[5]))
			continue;
		if (node_id) {
			memcpy(node_id, a, 6);
			close(sd);
			return 1;
		}
	}
	close(sd);
	return 0;
}

/* Assume that the gettimeofday() has microsecond granularity */
#define MAX_ADJUSTMENT 10

/*
 * Get clock from global sequence clock counter.
 *
 * Return -1 if the clock counter could not be opened/locked (in this case
 * pseudorandom value is returned in @ret_clock_seq), otherwise return 0.
 */
static int get_clock(uint32_t *clock_high, uint32_t *clock_low,
		     uint16_t *ret_clock_seq, int *num)
{
	THREAD_LOCAL int		adjustment = 0;
	THREAD_LOCAL struct timeval	last = {0, 0};
	THREAD_LOCAL int		state_fd = -2;
	THREAD_LOCAL FILE		*state_f;
	THREAD_LOCAL uint16_t		clock_seq;
	struct timeval			tv;
	uint64_t			clock_reg;
	mode_t				save_umask;
	int				len;
	int				ret = 0;

	if (state_fd == -1)
		ret = -1;

	if (state_fd == -2) {
		save_umask = umask(0);
		state_fd = open(LIBUUID_CLOCK_FILE, O_RDWR|O_CREAT|O_CLOEXEC, 0660);
		(void) umask(save_umask);
		if (state_fd != -1) {
			state_f = fdopen(state_fd, "r+e");
			if (!state_f) {
				close(state_fd);
				state_fd = -1;
				ret = -1;
			}
		}
		else
			ret = -1;
	}
	if (state_fd >= 0) {
		rewind(state_f);
		while (flock(state_fd, LOCK_EX) < 0) {
			if ((errno == EAGAIN) || (errno == EINTR))
				continue;
			fclose(state_f);
			close(state_fd);
			state_fd = -1;
			ret = -1;
			break;
		}
	}
	if (state_fd >= 0) {
		unsigned int cl;
		unsigned long tv1, tv2;
		int a;

		if (fscanf(state_f, "clock: %04x tv: %lu %lu adj: %d\n",
			   &cl, &tv1, &tv2, &a) == 4) {
			clock_seq = cl & 0x3FFF;
			last.tv_sec = tv1;
			last.tv_usec = tv2;
			adjustment = a;
		}
	}

	if ((last.tv_sec == 0) && (last.tv_usec == 0)) {
		random_get_bytes(&clock_seq, sizeof(clock_seq));
		clock_seq &= 0x3FFF;
		gettimeofday(&last, NULL);
		last.tv_sec--;
	}

try_again:
	gettimeofday(&tv, NULL);
	if ((tv.tv_sec < last.tv_sec) ||
	    ((tv.tv_sec == last.tv_sec) &&
	     (tv.tv_usec < last.tv_usec))) {
		clock_seq = (clock_seq+1) & 0x3FFF;
		adjustment = 0;
		last = tv;
	} else if ((tv.tv_sec == last.tv_sec) &&
	    (tv.tv_usec == last.tv_usec)) {
		if (adjustment >= MAX_ADJUSTMENT)
			goto try_again;
		adjustment++;
	} else {
		adjustment = 0;
		last = tv;
	}

	clock_reg = tv.tv_usec*10 + adjustment;
	clock_reg += ((uint64_t) tv.tv_sec)*10000000;
	clock_reg += (((uint64_t) 0x01B21DD2) << 32) + 0x13814000;

	if (num && (*num > 1)) {
		adjustment += *num - 1;
		last.tv_usec += adjustment / 10;
		adjustment = adjustment % 10;
		last.tv_sec += last.tv_usec / 1000000;
		last.tv_usec = last.tv_usec % 1000000;
	}

	if (state_fd >= 0) {
		rewind(state_f);
		len = fprintf(state_f,
			      "clock: %04x tv: %016ld %08ld adj: %08d\n",
			      clock_seq, (long)last.tv_sec, (long)last.tv_usec, adjustment);
		fflush(state_f);
		if (ftruncate(state_fd, len) < 0) {
			fprintf(state_f, "                   \n");
			fflush(state_f);
		}
		rewind(state_f);
		flock(state_fd, LOCK_UN);
	}

	*clock_high = clock_reg >> 32;
	*clock_low = clock_reg;
	*ret_clock_seq = clock_seq;
	return ret;
}

/*
 * Try using the uuidd daemon to generate the UUID
 *
 * Returns 0 on success, non-zero on failure.
 */
static int get_uuid_via_daemon(int op, uuid_t out, int *num)
{
	char op_buf[64];
	int op_len;
	int s;
	ssize_t ret;
	int32_t reply_len = 0, expected = 16;
	struct sockaddr_un srv_addr;

	if (sizeof(UUIDD_SOCKET_PATH) > sizeof(srv_addr.sun_path))
		return -1;

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		return -1;

	srv_addr.sun_family = AF_UNIX;
	strncpy(srv_addr.sun_path, UUIDD_SOCKET_PATH, sizeof(srv_addr.sun_path));

	if (connect(s, (const struct sockaddr *) &srv_addr,
		    sizeof(struct sockaddr_un)) < 0)
		goto fail;

	op_buf[0] = op;
	op_len = 1;
	if (op == UUIDD_OP_BULK_TIME_UUID) {
		memcpy(op_buf+1, num, sizeof(*num));
		op_len += sizeof(*num);
		expected += sizeof(*num);
	}

	ret = write(s, op_buf, op_len);
	if (ret < 1)
		goto fail;

	ret = read(s, (char *) &reply_len, sizeof(reply_len));
	if (ret < 0)
		goto fail;

	if (reply_len != expected)
		goto fail;

	ret = read(s, op_buf, reply_len);

	if (op == UUIDD_OP_BULK_TIME_UUID)
		memcpy(op_buf+16, num, sizeof(int));

	memcpy(out, op_buf, 16);

	close(s);
	return ((ret == expected) ? 0 : -1);

fail:
	close(s);
	return -1;
}

int __uuid_generate_time(uuid_t out, int *num)
{
	static unsigned char node_id[6];
	static int has_init = 0;
	struct uuid uu;
	uint32_t	clock_mid;
	int ret;

	if (!has_init) {
		if (get_node_id(node_id) <= 0) {
			random_get_bytes(node_id, 6);
			/*
			 * Set multicast bit, to prevent conflicts
			 * with IEEE 802 addresses obtained from
			 * network cards
			 */
			node_id[0] |= 0x01;
		}
		has_init = 1;
	}
	ret = get_clock(&clock_mid, &uu.time_low, &uu.clock_seq, num);
	uu.clock_seq |= 0x8000;
	uu.time_mid = (uint16_t) clock_mid;
	uu.time_hi_and_version = ((clock_mid >> 16) & 0x0FFF) | 0x1000;
	memcpy(uu.node, node_id, 6);
	uuid_pack(&uu, out);
	return ret;
}

/*
 * Generate time-based UUID and store it to @out
 *
 * Tries to guarantee uniqueness of the generated UUIDs by obtaining them from the uuidd daemon,
 * or, if uuidd is not usable, by using the global clock state counter (see get_clock()).
 * If neither of these is possible (e.g. because of insufficient permissions), it generates
 * the UUID anyway, but returns -1. Otherwise, returns 0.
 */
static int uuid_generate_time_generic(uuid_t out) {
	THREAD_LOCAL int		num = 0;
	THREAD_LOCAL struct uuid	uu;
	THREAD_LOCAL time_t		last_time = 0;
	time_t				now;

	if (num > 0) {
		now = time(NULL);
		if (now > last_time+1)
			num = 0;
	}
	if (num <= 0) {
		num = 1000000;
		if (get_uuid_via_daemon(UUIDD_OP_BULK_TIME_UUID,
					out, &num) == 0) {
			last_time = time(NULL);
			uuid_unpack(out, &uu);
			num--;
			return 0;
		}
		num = 0;
	}
	if (num > 0) {
		uu.time_low++;
		if (uu.time_low == 0) {
			uu.time_mid++;
			if (uu.time_mid == 0)
				uu.time_hi_and_version++;
		}
		num--;
		uuid_pack(&uu, out);
		return 0;
	}

	return __uuid_generate_time(out, NULL);
}

/*
 * Generate time-based UUID and store it to @out.
 *
 * Discards return value from uuid_generate_time_generic()
 */
void uuid_generate_time(uuid_t out)
{
	(void)uuid_generate_time_generic(out);
}


int uuid_generate_time_safe(uuid_t out)
{
	return uuid_generate_time_generic(out);
}


int __uuid_generate_random(uuid_t out, int *num)
{
	uuid_t	buf;
	struct uuid uu;
	int i, n, r = 0;

	if (!num || !*num)
		n = 1;
	else
		n = *num;

	for (i = 0; i < n; i++) {
		if (random_get_bytes(buf, sizeof(buf)))
			r = -1;
		uuid_unpack(buf, &uu);

		uu.clock_seq = (uu.clock_seq & 0x3FFF) | 0x8000;
		uu.time_hi_and_version = (uu.time_hi_and_version & 0x0FFF)
			| 0x4000;
		uuid_pack(&uu, out);
		out += sizeof(uuid_t);
	}

	return r;
}

void uuid_generate_random(uuid_t out)
{
	int	num = 1;
	/* No real reason to use the daemon for random uuid's -- yet */

	__uuid_generate_random(out, &num);
}

/*
 * This is the generic front-end to __uuid_generate_random and
 * uuid_generate_time.  It uses __uuid_generate_random output
 * only if high-quality randomness is available.
 */
void uuid_generate(uuid_t out)
{
	int num = 1;

	if (__uuid_generate_random(out, &num))
		uuid_generate_time(out);
}
