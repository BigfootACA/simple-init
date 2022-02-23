/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_ZLIB
#include<zlib.h>
#include<stdlib.h>
#include<string.h>
#ifdef ENABLE_UEFI
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#endif
#include"internal.h"
#include"logger.h"
#define TAG "gzip"

static void zfree(voidpf q __attribute__((unused)),void*a){
	#ifdef ENABLE_UEFI
	FreePool(a);
	#else
	free(a);
	#endif
}

static void*zalloc(voidpf q __attribute__((unused)),uInt cnt,uInt size){
	void*r;
	size_t s=cnt*size;
	#ifdef ENABLE_UEFI
	r=AllocatePool(s);
	if(r)ZeroMem(r,s);
	#else
	r=malloc(s);
	if(r)memset(r,0,s);
	#endif
	return r;
}

static int gunzip(
	unsigned char*inp,size_t inp_len,
	unsigned char*out,size_t out_len,
	size_t*pos,size_t*len
){
	int ret=0;
	struct z_stream_s zs;
	if(inp_len<=10||out_len<=inp_len)return -1;
	zs.zalloc=zalloc;
	zs.zfree=zfree;
	zs.next_out=(Bytef*)out;
	zs.avail_out=(uInt)out_len;
	zs.next_in=(Bytef*)inp+10;
	zs.avail_in=(uInt)inp_len-10;
	if(inp[3]&0x8)for(int i=0;i<256&&*(zs.next_in++);i++){
		if(zs.avail_in<=0)return -1;
		zs.avail_in--;
	}
	if((inflateInit2(&zs,-MAX_WBITS))!=Z_OK){
		tlog_error("zlib inflate init failed!");
		return -1;
	}
	switch(inflate(&zs,Z_NO_FLUSH)){
		case Z_STREAM_END:break;
		case Z_OK:
			if(zs.avail_out>0)break;
			tlog_error("zlib output buffer full");
			ret=-1;
		break;
		default:
			tlog_error("zlib unknown error");
			ret=-1;
		break;
	}
	inflateEnd(&zs);
	if(pos)*pos=zs.next_in-inp+8;
	if(len)*len=zs.total_out;
	return ret;
}

static bool is_gzip(unsigned char*inp,size_t inp_len){
	return inp_len>10&&inp[0]==0x1f&&inp[1]==0x8b&&inp[2]==0x08;
}

compressor compressor_gzip={
	.name="gzip",
	.ext="gz",
	.mime="application/gzip",
	.is_format=is_gzip,
	.compress=NULL,
	.decompress=gunzip
};
#endif
