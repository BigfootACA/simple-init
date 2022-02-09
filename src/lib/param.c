/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<string.h>
#include<unistd.h>
#include<stdbool.h>
#include"defines.h"
#include"keyval.h"
#include"param.h"

keyval**param_s_parse_items(char*cmdline,size_t len,size_t*length){
	static keyval items[256];
	static keyval*pointers[256];
	static char buffer[BUFSIZ];
	size_t i,pos=0,item=0;
	char quote,*buff = buffer;
	bool onkey=true;
	if(!cmdline||len<=0)return NULL;
	memset(items,0,sizeof(items));
	memset(buffer,0,sizeof(buffer));
	memset(pointers,0,sizeof(pointers));
	for(i=0;i<len;i++)switch(cmdline[i]){
		/* Quote */
		case '"':case '\'':
			quote=cmdline[i];
			while(true){
				i++;
				if(i>=len)break;// Unexpected end
				if(cmdline[i]==quote)break;// Found close quote
				if(pos>=sizeof(buffer)-1)break;// Out of buffer
				buffer[pos]=cmdline[i];
				pos++;
			}
		break;
		/* Terminator */
		case ' ':case '\t':case '\n':case '\r':case '#':
			if(*buff){
				if(onkey)items[item].key=buff;
				else items[item].value=buff;
				pos++;
				buff=buffer+pos;
			}
			if(items[item].key||items[item].value)item++;
			onkey=true;
		break;
		/* Argument */
		case '=':
			if(onkey){
				onkey=false;
				items[item].key=buff;
				pos++,buff=buffer+pos;
				break;
			}
			// fallthrough
		/* Standard chars */
		default:
			if(pos>=sizeof(buffer))break;
			buffer[pos++]=cmdline[i];
	}
	if(*buff){
		if(onkey)items[item].key=buff;
		else items[item].value=buff;
	}
	if(items[item].key||items[item].value)item++;
	if(length)*length=item;
	for(i=0;i<item;i++)pointers[i]=&items[i];
	return pointers;
}

keyval**param_parse_items(char*cmdline,size_t*length){
	return param_s_parse_items(cmdline,strlen(cmdline),length);
}

#ifndef ENABLE_UEFI
keyval**read_params(int fd){
	static char buffer[BUFSIZ];
	if(fd<0)EPRET(EBADF);
	memset(buffer,0,sizeof(buffer));
	ssize_t len=read(fd,buffer,sizeof(buffer)-1);
	if(len<=0)return NULL;
	if(buffer[sizeof(buffer)-1]!=0)EPRET(EFAULT);
	return param_s_parse_items(buffer,(size_t)len,NULL);
}
#endif
