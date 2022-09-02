/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _COMPRESS_H
#define _COMPRESS_H
#include<sys/types.h>

typedef struct compressor compressor;

// src/compress/compress.c: get compressor name
extern const char*compressor_get_name(compressor*c);

// src/compress/compress.c: get compressor supported mime
extern const char*compressor_get_mime(compressor*c);

// src/compress/compress.c: get compressor supported file ext name
extern const char*compressor_get_ext(compressor*c);

// src/compress/compress.c: get compressor by compressor name
extern compressor*compressor_get_by_name(const char*name);

// src/compress/compress.c: get compressor by file ext name
extern compressor*compressor_get_by_ext(const char*ext);

// src/compress/compress.c: get compressor by file ext name
extern compressor*compressor_get_by_filename(const char*file);

// src/compress/compress.c: get compressor by file mime
extern compressor*compressor_get_by_mime(const char*mime);

// src/compress/compress.c: get compressor by file format
extern compressor*compressor_get_by_format(unsigned char*data,size_t len);

// src/compress/compress.c: check target is compressed data
extern bool compressor_is_compressed(unsigned char*data,size_t len);

// src/compress/compress.c: do compress
extern int compressor_compress(
	compressor*c,
	unsigned char*inp,size_t inp_len,
	unsigned char*out,size_t out_len,
	size_t*pos,size_t*len
);

// src/compress/compress.c: do decompress
extern int compressor_decompress(
	compressor*c,
	unsigned char*inp,size_t inp_len,
	unsigned char*out,size_t out_len,
	size_t*pos,size_t*len
);
#define DECL_INLINE(suffix,param...)\
	extern int compressor_compress_##suffix(\
		param\
		unsigned char*inp,size_t inp_len,\
		unsigned char*out,size_t out_len,\
		size_t*pos,size_t*len\
	);\
	extern int compressor_decompress_##suffix(\
		param\
		unsigned char*inp,size_t inp_len,\
		unsigned char*out,size_t out_len,\
		size_t*pos,size_t*len\
	);

// src/compress/compress.c: detect format and auto compress/decompress
DECL_INLINE(auto,)

// src/compress/compress.c: compress/decompress by compressor name
DECL_INLINE(by_name,const char*name,)

// src/compress/compress.c: compress/decompress by file ext name
DECL_INLINE(by_ext,const char*ext,)

// src/compress/compress.c: compress/decompress by file ext name
DECL_INLINE(by_filename,const char*filename,)

// src/compress/compress.c: compress/decompress by file mime
DECL_INLINE(by_mime,const char*mime,)

#endif
