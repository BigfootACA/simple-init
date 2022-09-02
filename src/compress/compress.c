/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<string.h>
#include<strings.h>
#include"internal.h"
#include"defines.h"

const char*compressor_get_name(compressor*c){
	return c?c->name:NULL;
}

const char*compressor_get_mime(compressor*c){
	return c?c->mime:NULL;
}

const char*compressor_get_ext(compressor*c){
	return c?c->ext:NULL;
}

compressor*compressor_get_by_name(const char*name){
	compressor*c=NULL;
	if(!name||!name[0])EPRET(EINVAL);
	for(size_t i=0;(c=compressors[i]);i++)
		if(strncasecmp(c->name,name,sizeof(c->name)-1))
			return c;
	EPRET(ENOENT);
}

compressor*compressor_get_by_ext(const char*ext){
	compressor*c=NULL;
	if(!ext||!ext[0])EPRET(EINVAL);
	for(size_t i=0;(c=compressors[i]);i++)
		if(strncasecmp(c->ext,ext,sizeof(c->ext)-1))
			return c;
	EPRET(ENOENT);
}

compressor*compressor_get_by_filename(const char*file){
	char*r=strrchr(file,'.');
	if(!r||strchr(r,'/')||!r[1])EPRET(ENOTSUP);
	return compressor_get_by_ext(r+1);
}

compressor*compressor_get_by_mime(const char*mime){
	compressor*c=NULL;
	if(!mime||!mime[0])EPRET(EINVAL);
	for(size_t i=0;(c=compressors[i]);i++)
		if(strncasecmp(c->mime,mime,sizeof(c->mime)-1))
			return c;
	EPRET(ENOENT);
}

compressor*compressor_get_by_format(unsigned char*data,size_t len){
	compressor*c=NULL;
	if(!data||len<=0)EPRET(EINVAL);
	for(size_t i=0;(c=compressors[i]);i++)
		if(c->is_format&&c->is_format(data,len))
			return c;
	EPRET(ENOENT);
}

bool compressor_is_compressed(unsigned char*data,size_t len){
	compressor*c=NULL;
	if(!data||len<=0)return false;
	for(size_t i=0;(c=compressors[i]);i++)
		if(c->is_format&&c->is_format(data,len))
			return true;
	return false;
}

int compressor_compress(
	compressor*c,
	unsigned char*inp,size_t inp_len,
	unsigned char*out,size_t out_len,
	size_t*pos,size_t*len
){
	if(!c||!inp||!out||inp_len<=0||out_len<=0)ERET(EINVAL);
	if(!c->compress)ERET(ENOSYS);
	return c->compress(inp,inp_len,out,out_len,pos,len);
}

int compressor_decompress(
	compressor*c,
	unsigned char*inp,size_t inp_len,
	unsigned char*out,size_t out_len,
	size_t*pos,size_t*len
){
	if(!c||!inp||!out||inp_len<=0||out_len<=0)ERET(EINVAL);
	if(!c->decompress)ERET(ENOSYS);
	return c->decompress(inp,inp_len,out,out_len,pos,len);
}

#define IMPL_INLINE(suffix,by,arg,param...)\
	int compressor_compress_##suffix(\
		param\
		unsigned char*inp,size_t inp_len,\
		unsigned char*out,size_t out_len,\
		size_t*pos,size_t*len\
	){\
		compressor*c=compressor_get_by_##by arg;\
		if(!c)ERET(ENOTSUP);\
		return compressor_compress(c,inp,inp_len,out,out_len,pos,len);\
	}\
	int compressor_decompress_##suffix(\
		param\
		unsigned char*inp,size_t inp_len,\
		unsigned char*out,size_t out_len,\
		size_t*pos,size_t*len\
	){\
		compressor*c=compressor_get_by_##by arg;\
		if(!c)ERET(ENOTSUP);\
		return compressor_decompress(c,inp,inp_len,out,out_len,pos,len);\
	}

IMPL_INLINE(auto,format,(inp,inp_len),)
IMPL_INLINE(by_name,name,(name),const char*name,)
IMPL_INLINE(by_ext,ext,(ext),const char*ext,)
IMPL_INLINE(by_filename,filename,(filename),const char*filename,)
IMPL_INLINE(by_mime,mime,(mime),const char*mime,)
