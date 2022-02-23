/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _COMPRESS_INTERNAL_H
#define _COMPRESS_INTERNAL_H
#include<stdbool.h>
#include<sys/types.h>
#include"compress.h"

typedef int(*compress_func)(
	unsigned char*inp,size_t inp_len,
	unsigned char*out,size_t out_len,
	size_t*pos,size_t*len
);

typedef bool(*check_func)(unsigned char*inp,size_t inp_len);

struct compressor{
	const char name[256];
	const char ext[32];
	const char mime[64];
	check_func is_format;
	compress_func compress;
	compress_func decompress;
};

extern compressor*compressors[];
#endif
