/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<string.h>
#include"defines.h"
#include"keyval.h"
#include"str.h"

char*replace(keyval**table,char del,char*dest,char*src,size_t size){
	if(!src||!dest)return NULL;
	memset(dest,0,size);
	size_t sc=0,dc=0;
	char prev=0,sv[PATH_MAX];
	memset(sv,0,1024);
	strncpy(sv,src,1023);
	for(;;){
		if(sv[sc]==0||size<=dc)break;
		if(prev==del){
			if(sv[sc]==del)dest[dc++]='\\',sv[sc]=' ';
			else{
				KVARR_FOREACH(table,t,i){
					if(!t->key||t->key[1]!=0||!t->value)return NULL;
					if(sv[sc]==t->key[0]){
						strncpy(dest+dc,t->value,size-dc);
						dc+=strlen(t->value);
						sv[sc]=0;
						break;
					}
				}
				if(sv[sc]!=0){
					strncpy(dest+dc,(char[]){del,sv[sc],0},size-dc);
					dc+=2;
				}
			}
		}else if(sv[sc]!=del)dest[dc++]=sv[sc];
		prev=sv[sc];
		sc++;
	}
	return dest;
}
