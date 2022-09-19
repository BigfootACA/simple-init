/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<string.h>
#include"confd_internal.h"
#include"logger.h"
#define TAG "config"

extern struct conf_file_hand conf_hand_conf;
extern struct conf_file_hand conf_hand_json;
extern struct conf_file_hand conf_hand_xml;
struct conf_file_hand*conf_hands[]={
	&conf_hand_conf,
	#ifdef ENABLE_JSONC
	&conf_hand_json,
	#endif
	#ifdef ENABLE_MXML
	&conf_hand_xml,
	#endif
	NULL
};

static const char*file_get_ext(const char*path){
	size_t s=strlen(path);
	if(s==0)EPRET(EINVAL);
	for(size_t i=s-1;i>0;i--){
		if(path[i]=='.')return path+i+1;
		if(path[i]=='/')break;
	}
	return NULL;
}

static struct conf_file_hand*find_hand(const char*ext){
	if(!ext)EPRET(EINVAL);
	char*e;
	struct conf_file_hand*fh;
	for(size_t s=0;(fh=conf_hands[s]);s++){
		if(!fh->ext)continue;
		for(size_t x=0;(e=fh->ext[x]);x++)
			if(strcasecmp(e,ext)==0)return fh;
	}
	EPRET(ENOENT);
}

static struct conf_file_hand*find_hand_by_file(const char*file){
	return find_hand(file_get_ext(file));
}

static int do_close(struct conf_file_hand*hand,bool save){
	int r=0;
	if(save&&hand->off>0){
		fs_seek(hand->file,0,SEEK_SET);
		r=fs_full_write(hand->file,hand->buff,hand->off);
		if(r!=0)tlog_warn("write config failed: %s",strerror(r));
	}
	if(hand->file)fs_close(&hand->file);
	if(hand->buff)free(hand->buff);
	hand->file=NULL,hand->buff=NULL,hand->len=0,hand->off=0;
	return r;
}

static int do_load(struct conf_file_hand*hand){
	int r=0;
	do_close(hand,false);
	hand->file=NULL,hand->len=0;
	r=fs_open(hand->parent,&hand->file,hand->path,FILE_FLAG_READ);
	if(r!=0)EDONE(if(r!=ENOENT)tlog_warn("open config '%s' failed: %s",hand->path,strerror(r)));
	r=fs_get_size(hand->file,&hand->len);
	if(hand->len>0x400000)EDONE(tlog_warn("config '%s' too large",hand->path));
	if(hand->len<=0)EDONE(tlog_warn("config '%s' too small",hand->path));
	r=fs_read_all(hand->file,(void**)&hand->buff,&hand->len);
	if(r<0)EDONE(telog_warn("read '%s' failed",hand->path));
	return 0;
	done:do_close(hand,false);
	return errno<0?-(errno):-1;
}

static int do_save(struct conf_file_hand*hand){
	int r=0;
	do_close(hand,false);
	r=fs_open(hand->parent,&hand->file,hand->path,FILE_FLAG_WRITE|FILE_FLAG_CREATE|0644);
	if(r!=0)EDONE(telog_warn("open config '%s' failed: %s",hand->path,strerror(r)));
	fs_seek(hand->file,0,SEEK_SET);
	fs_set_size(hand->file,0);
	return 0;
	done:do_close(hand,false);
	return errno<0?-(errno):-1;
}

static ssize_t conf_read(struct conf_file_hand*hand,char*buff,size_t len){
	if(!hand||!buff||len<=0)return -1;
	memset(buff,0,len);
	size_t avail=hand->len-hand->off;
	size_t process=MIN(avail,len);
	if(avail==0)return 0;
	memcpy(buff,hand->buff+hand->off,process);
	hand->off+=process;
	return process;
}

static ssize_t conf_write(struct conf_file_hand*hand,char*buff,size_t len){
	if(!hand||!buff)return -1;
	if(len<=0)len=strlen(buff);
	if(len>hand->len-hand->off-1||!hand->buff){
		do{hand->len+=4096;}while(len>hand->len-hand->off-1);
		if(hand->buff){
			void*buf=realloc(hand->buff,hand->len);
			if(!buf)return -1;
			hand->buff=buf;
		}else if(!(hand->buff=malloc(hand->len)))return -1;
	}
	memcpy(hand->buff+hand->off,buff,len);
	hand->off+=len;
	return len;
}

static int _conf_load_file(fsh*parent,const char*file,bool inc,int depth){
	int r=0;
	static char xpath[PATH_MAX];
	if(depth>=8)ERET(ELOOP);
	if(!file)ERET(EINVAL);
	struct conf_file_hand*hand=find_hand_by_file(file);
	if(!hand)ERET(EINVAL);
	if(!hand->load)ERET(ENOSYS);
	if(!hand->initialized){
		MUTEX_INIT(hand->lock);
		hand->initialized=true;
	}
	MUTEX_LOCK(hand->lock);
	memset(xpath,0,sizeof(xpath));
	strncpy(xpath,file,sizeof(xpath)-1);
	hand->include=inc;
	hand->parent=parent;
	hand->write=NULL;
	hand->read=conf_read;
	hand->path=xpath;
	hand->depth=depth;
	r=do_load(hand);
	if(r==0){
		r=hand->load(hand);
		do_close(hand,false);
		if(r==0)errno=0;
	}else r=-1;
	hand->include=false;
	hand->parent=NULL;
	hand->write=NULL;
	hand->read=NULL;
	hand->path=NULL;
	MUTEX_UNLOCK(hand->lock);
	return r;
}

char**conf_get_supported_exts(){
	char**exts=NULL;
	size_t cnt=0,size;
	struct conf_file_hand*h;
	for(size_t x=0;(h=conf_hands[x]);x++)
		if(h->ext)for(size_t y=0;h->ext[y];y++)cnt++;
	size=(cnt+1)*sizeof(char*),cnt=0;
	if(!(exts=malloc(size)))return NULL;
	memset(exts,0,size);
	for(size_t x=0;(h=conf_hands[x]);x++)
		if(h->ext)for(size_t y=0;h->ext[y];y++)
			exts[cnt++]=h->ext[y];
	return exts;
}

int conf_load_file(fsh*parent,const char*file){
	return _conf_load_file(parent,file,false,0);
}

int conf_include_file(fsh*parent,const char*file){
	return _conf_load_file(parent,file,true,0);
}

int conf_include_file_depth(fsh*parent,const char*file,int depth){
	return _conf_load_file(parent,file,true,depth);
}

int conf_save_file(fsh*parent,const char*file){
	int r=0;
	static char xpath[PATH_MAX];
	if(!file)ERET(EINVAL);
	struct conf_file_hand*hand=find_hand_by_file(file);
	if(!hand)ERET(EINVAL);
	if(!hand->save)ERET(ENOSYS);
	if(!hand->initialized){
		MUTEX_INIT(hand->lock);
		hand->initialized=true;
	}
	MUTEX_LOCK(hand->lock);
	memset(xpath,0,sizeof(xpath));
	strncpy(xpath,file,sizeof(xpath)-1);
	hand->parent=parent;
	hand->write=conf_write;
	hand->read=NULL;
	hand->path=xpath;
	r=do_save(hand);
	if(r==0){
		r=hand->save(hand);
		do_close(hand,true);
		if(r==0)errno=0;
	}else{
		tlog_warn("save failed");
		r=-1;
	}
	hand->include=false;
	hand->parent=NULL;
	hand->write=NULL;
	hand->read=NULL;
	hand->path=NULL;
	MUTEX_UNLOCK(hand->lock);
	return r;
}
