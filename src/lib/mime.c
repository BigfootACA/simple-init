/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stdint.h>
#include"str.h"
#include"assets.h"
#define _MIMES "/usr/share/mime/mime.types"
#define _MIME_FAIL "application/octet-stream"

char*mime_get_by_ext(char*buff,size_t bs,const char*ext){
	if(!ext||!buff||bs<=0)return NULL;
	memset(buff,0,bs);
	entry_file*file=rootfs_get_assets_file(_MIMES);
	if(!file||!file->content)
		return strncpy(buff,_MIME_FAIL,bs-1);
	size_t l=file->length,p=0;
	char*c=file->content;
	char buf[512];
	uint8_t m=0;
	memset(buf,0,sizeof(buf));
	for(size_t i=0;i<l;i++)switch(c[i]){
		case '#':while(i<l&&c[i]!='\r'&&c[i]!='\n')i++;break;
		case '\r':case '\n':
			if(m==1&&strcasecmp(buf,ext)==0&&p>0)
				return buff;
			memset(buf,0,sizeof(buf));
			m=0,p=0;
		break;
		case '\t':case ' ':
			if(m==1&&strcasecmp(buf,ext)==0&&p>0)
				return buff;
			else if(m==0&&p>0){
				memset(buff,0,bs);
				strncpy(buff,buf,bs-1);
				m=1,p=0;
			}
			memset(buf,0,sizeof(buf));
		break;
		default:buf[p++]=c[i];break;
	}
	return strncpy(buff,_MIME_FAIL,bs-1);
}

char*mime_get_by_filename(char*buff,size_t bs,const char*filename){
	if(!filename)return NULL;
	char*p=strrchr(filename,'.');
	if(!p)return strncpy(buff,_MIME_FAIL,bs-1);
	return mime_get_by_ext(buff,bs,p+1);
}
