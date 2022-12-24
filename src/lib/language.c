/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<fcntl.h>
#include<stdio.h>
#include<limits.h>
#include<unistd.h>
#include<string.h>
#include<stdint.h>
#include<stdlib.h>
#include<sys/stat.h>
#ifndef ENABLE_UEFI
#include<sys/mman.h>
#endif
#include"str.h"
#include"shell.h"
#include"confd.h"
#include"assets.h"
#include"defines.h"
#include"version.h"
#include"pathnames.h"
#include"libmo.h"
#include"language.h"
#define DEFAULT_LOCALE _PATH_USR"/share/locale"

struct language languages[]={
	{"en","US","UTF-8","English (USA)"},
	{"zh","CN","UTF-8","简体中文 (中国大陆)"},
	{0,0,0,0}
};

#ifdef ENABLE_UEFI
static char cur_lang[64]={0};
#endif
static bool mmap_map=false;
static void*locale_map=NULL;
static size_t map_size=-1;
static struct libmo_context moctx;

static int lang_open_locale(char*path){
	int r=-1;
	#ifndef ENABLE_UEFI
	int fd=-1;
	struct stat st;
	if(locale_map&&mmap_map)munmap(locale_map,map_size);
	#endif
	locale_map=NULL;
	map_size=-1;
	struct entry_file*file=rootfs_get_assets_file(path);
	if(file
		&&S_ISREG(file->info.mode)
		#ifndef ENABLE_UEFI
		&&!getenv("NO_INTERNAL_MO")
		#endif
	){
		locale_map=file->content;
		map_size=file->length;
		mmap_map=false;
		r=libmo_load(&moctx, locale_map, map_size);
		if(r)goto clean;
		r=libmo_verify(&moctx);
		if(r)goto clean;
	#ifndef ENABLE_UEFI
	}else if((fd=open(path,O_RDONLY))<0)goto clean;
	else{
		if(fstat(fd,&st)!=0)goto clean;
		if((map_size=st.st_size)<=0)goto clean;
		if(!(locale_map=mmap(NULL,map_size,PROT_READ,MAP_SHARED,fd,0))){
			map_size=-1;
			goto clean;
		}
		mmap_map=true;
		r=libmo_load(&moctx, locale_map, map_size);
		if(r)goto clean;
		r=libmo_verify(&moctx);
		if(r)goto clean;
	#endif
	}
	if(locale_map)r=0;
	#ifndef ENABLE_UEFI
	clean:
	if(fd>=0)close(fd);
	#endif
	return r;
}

char*lang_get_locale(char*def){
	if(def)return def;
	#ifdef ENABLE_UEFI
	if(!cur_lang[0])strcpy(cur_lang,confd_get_string("language","C"));
	return strdup(cur_lang);
	#else
	char*l;
	if((l=getenv("LC_ALL")))return strdup(l);
	if((l=getenv("LANG")))return strdup(l);
	if((l=getenv("LANGUAGE")))return strdup(l);
	if((l=confd_get_string("language","C")))return l;
	return NULL;
	#endif
}

void lang_load_locale(const char*dir,const char*lang,const char*domain){
	if(!domain)return;
	char rl[64],path[PATH_MAX],*p;
	memset(rl,0,64);
	char*l=lang_get_locale((char*)lang);
	if(!l)return;
	strncpy(rl,l,63);
	free(l);
	char*d=dir?(char*)dir:DEFAULT_LOCALE;
	for(;;){
		memset(path,0,PATH_MAX);
		snprintf(path,PATH_MAX-1,"%s/%s/LC_MESSAGES/%s.mo",d,rl,domain);
		if(lang_open_locale(path)==0)break;
		if((p=strchr(rl,'.')))*p=0;
		else if((p=strchr(rl,'_')))*p=0;
		else break;
	}
}

char*lang_gettext(const char*msgid){
	if(!locale_map||!msgid)return (char*)msgid;
	return (char*)libmo_gettext(&moctx, msgid);
}

void lang_init_locale(){
	lang_load_locale(NULL,NULL,NAME);
	#ifndef ENABLE_UEFI
	init_commands_locale();
	#endif
}

const char*lang_concat(struct language*lang,bool region,bool charset){
	if(!lang)return NULL;
	static char c[32];
	char*p=c;
	memset(c,0,32);
	strncpy(p,lang->lang,4);
	if(region&&lang->region[0]){
		while(*(++p)!=0);
		*(p++)='_';
		strncpy(p,lang->region,4);
	}
	if(charset&&lang->charset[0]){
		while(*(++p)!=0);
		*(p++)='.';
		strncpy(p,lang->charset,16);
	}
	return c;
}

bool lang_compare(struct language*lang,const char*name){
	return lang&&name&&(
		strcmp(name,lang_concat(lang,false,false))==0||
		strcmp(name,lang_concat(lang,false,true))==0||
		strcmp(name,lang_concat(lang,true,false))==0||
		strcmp(name,lang_concat(lang,true,true))==0
	);
}

int lang_set(const char*lang){
	if(
		!lang||lang[0]==0||
		!check_valid((char*)lang,VALID"-.")
	)ERET(EINVAL);
	#ifdef ENABLE_UEFI
	memset(cur_lang,0,sizeof(cur_lang));
	strcpy(cur_lang,lang);
	#else
	setenv("LANG",lang,1);
	setenv("LANGUAGE",lang,1);
	setenv("LC_ALL",lang,1);
	#endif
	confd_set_string("language",(char*)lang);
	lang_init_locale();
	return 0;
}
