/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fs_internal.h"

int fs_write_file_uri(
	url*uri,
	void*buffer,
	size_t len
){
	int r=0;
	fsh*hand=NULL;
	if(!uri||!buffer)RET(EINVAL);
	r=fs_open_uri(
		&hand,uri,
		FILE_FLAG_WRITE|
		FILE_FLAG_TRUNCATE|
		FILE_FLAG_CREATE|
		0644
	);
	if(r!=0)XRET(r,EIO);
	r=fs_full_write(hand,buffer,len);
	fs_flush(hand);
	fs_close(&hand);
	RET(r);
}

int fs_write_file(
	fsh*f,
	const char*path,
	void*buffer,
	size_t len
){
	int r=0;
	fsh*hand=NULL;
	if(!path||!buffer)RET(EINVAL);
	r=fs_open(
		f,&hand,path,
		FILE_FLAG_WRITE|
		FILE_FLAG_TRUNCATE|
		FILE_FLAG_CREATE|
		0644
	);
	if(r!=0)XRET(r,EIO);
	r=fs_full_write(hand,buffer,len);
	fs_flush(hand);
	fs_close(&hand);
	RET(r);
}

int fs_vprintf_file(
	fsh*f,
	const char*path,
	const char*format,
	va_list ap
){
	int r;
	ssize_t len=0;
	char*buf=NULL;
	if(!path||!format)RET(EINVAL);
	len=vasprintf(&buf,format,ap);
	if(len<0||!buf)RET(EINVAL);
	r=fs_write_file(f,path,buf,len);
	free(buf);
	RET(r);
}

int fs_vprintf_file_uri(
	url*uri,
	const char*format,
	va_list ap
){
	int r;
	ssize_t len=0;
	char*buf=NULL;
	if(!uri||!format)RET(EINVAL);
	len=vasprintf(&buf,format,ap);
	if(len<0||!buf)RET(EINVAL);
	r=fs_write_file_uri(uri,buf,len);
	free(buf);
	RET(r);
}

int fs_printf_file(
	fsh*f,
	const char*path,
	const char*format,
	...
){
	int r;
	va_list ap;
	if(!format)RET(EINVAL);
	va_start(ap,format);
	r=fs_vprintf_file(f,path,format,ap);
	va_end(ap);
	RET(r);
}

int fs_printf_file_uri(
	url*uri,
	const char*format,
	...
){
	int r;
	va_list ap;
	if(!format)RET(EINVAL);
	va_start(ap,format);
	r=fs_vprintf_file_uri(uri,format,ap);
	va_end(ap);
	RET(r);
}

int fs_read_file_uri(
	url*uri,
	void*buffer,
	size_t len
){
	int r=0;
	fsh*hand=NULL;
	if(!uri||!buffer)RET(EINVAL);
	r=fs_open_uri(&hand,uri,FILE_FLAG_READ);
	if(r!=0)XRET(r,EIO);
	r=fs_full_read(hand,buffer,len);
	fs_close(&hand);
	RET(r);
}

int fs_read_file(
	fsh*f,
	const char*path,
	void*buffer,
	size_t len
){
	int r=0;
	fsh*hand=NULL;
	if(!path||!buffer)RET(EINVAL);
	r=fs_open(f,&hand,path,FILE_FLAG_READ);
	if(r!=0)XRET(r,EIO);
	r=fs_full_read(hand,buffer,len);
	fs_close(&hand);
	RET(r);
}

int fs_read_whole_file_uri(
	url*uri,
	void**buffer,
	size_t*len
){
	int r=0;
	fsh*hand=NULL;
	if(!uri||!buffer||!len)RET(EINVAL);
	r=fs_open_uri(&hand,uri,FILE_FLAG_READ);
	if(r!=0)XRET(r,EIO);
	r=fs_read_all(hand,buffer,len);
	fs_close(&hand);
	RET(r);
}

int fs_read_whole_file(
	fsh*f,
	const char*path,
	void**buffer,
	size_t*len
){
	int r=0;
	fsh*hand=NULL;
	if(!path||!buffer)RET(EINVAL);
	r=fs_open(f,&hand,path,FILE_FLAG_READ);
	if(r!=0)XRET(r,EIO);
	r=fs_read_all(hand,buffer,len);
	fs_close(&hand);
	RET(r);
}
