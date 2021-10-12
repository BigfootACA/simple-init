#define _GNU_SOURCE
#include<string.h>
#include"confd_internal.h"

extern struct conf_file_hand conf_hand_conf;
struct conf_file_hand*conf_hands[]={
	&conf_hand_conf,
	NULL
};

#ifdef ENABLE_UEFI
#include<Protocol/SimpleFileSystem.h>
#endif

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

int conf_load_file(_ROOT_TYPE dir,const char*file){
	if(!file)ERET(EINVAL);
	#ifdef ENABLE_UEFI
	if(!dir)ERET(EINVAL);
	#else
	if(dir<0&&dir!=AT_FDCWD)ERET(EINVAL);
	#endif
	struct conf_file_hand*hand=find_hand_by_file(file);
	if(!hand)ERET(EINVAL);
	if(!hand->load)ERET(ENOSYS);
	return hand->load(dir,file);
}

int conf_save_file(_ROOT_TYPE dir,const char*file){
	if(!file)ERET(EINVAL);
	#ifdef ENABLE_UEFI
	if(!dir)ERET(EINVAL);
	#else
	if(dir<0&&dir!=AT_FDCWD)ERET(EINVAL);
	#endif
	struct conf_file_hand*hand=find_hand_by_file(file);
	if(!hand)ERET(EINVAL);
	if(!hand->save)ERET(ENOSYS);
	return hand->save(dir,file);
}
