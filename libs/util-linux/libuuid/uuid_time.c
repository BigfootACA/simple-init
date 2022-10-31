/*
 * uuid_time.c --- Interpret the time field from a uuid.  This program
 * 	violates the UUID abstraction barrier by reaching into the guts
 *	of a UUID and interpreting it.
 *
 * Copyright (C) 1998, 1999 Theodore Ts'o.
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

#include <sys/time.h>
#include "uuidP.h"

time_t uuid_time(const uuid_t uu, struct timeval *ret_tv)
{
	struct timeval		tv;
	struct uuid		uuid;
	uint32_t		high;
	uint64_t		clock_reg;

	uuid_unpack(uu, &uuid);

	high = uuid.time_mid | ((uuid.time_hi_and_version & 0xFFF) << 16);
	clock_reg = uuid.time_low | ((uint64_t) high << 32);

	clock_reg -= (((uint64_t) 0x01B21DD2) << 32) + 0x13814000;
	tv.tv_sec = clock_reg / 10000000;
	tv.tv_usec = (clock_reg % 10000000) / 10;

	if (ret_tv)
		*ret_tv = tv;

	return tv.tv_sec;
}

int uuid_type(const uuid_t uu)
{
	struct uuid		uuid;

	uuid_unpack(uu, &uuid);
	return ((uuid.time_hi_and_version >> 12) & 0xF);
}

int uuid_variant(const uuid_t uu)
{
	struct uuid		uuid;
	int			var;

	uuid_unpack(uu, &uuid);
	var = uuid.clock_seq;

	if ((var & 0x8000) == 0)
		return UUID_VARIANT_NCS;
	if ((var & 0x4000) == 0)
		return UUID_VARIANT_DCE;
	if ((var & 0x2000) == 0)
		return UUID_VARIANT_MICROSOFT;
	return UUID_VARIANT_OTHER;
}
