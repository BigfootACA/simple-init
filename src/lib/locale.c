#define _GNU_SOURCE
#include<fcntl.h>
#include<stdio.h>
#include<locale.h>
#include<limits.h>
#include<unistd.h>
#include<string.h>
#include<stdint.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include"shell.h"
#include"version.h"
#include"pathnames.h"
#include"language.h"
#define DEFAULT_LOCALE _PATH_USR"/share/locale"
static void*locale_map=NULL;
static size_t map_size=-1;
// swapc and mo_lookup from musl libc
static inline uint32_t swapc(uint32_t x,int c){return c?x>>24|(x>>8&0xff00)|(x<<8&0xff0000)|x<<24:x;}
static char*mo_lookup(const void*p,size_t size,const char*s){
	const uint32_t*mo=p;
	int sw=*mo-0x950412de;
	uint32_t b=0,n=swapc(mo[2],sw),o=swapc(mo[3],sw),t=swapc(mo[4],sw);
	if(n>=size/4||o>=size-4*n||t>=size-4*n||((o|t)%4))return 0;
	o/=4,t/=4;
	for(;;){
		uint32_t ol=swapc(mo[o+2*(b+n/2)],sw),os=swapc(mo[o+2*(b+n/2)+1],sw);
		if(os>=size||ol>=size-os||((char*)p)[os+ol])return 0;
		int sign=strcmp(s,(char*)p+os);
		if(!sign){
			uint32_t tl=swapc(mo[t+2*(b+n/2)],sw),ts=swapc(mo[t+2*(b+n/2)+1],sw);
			if(ts>=size||tl>=size-ts||((char*)p)[ts+tl])return 0;
			return(char*)p+ts;
		}else if(n==1)return 0;
		else if(sign<0)n/=2;
		else b+=n/2,n-=n/2;
	}
}
static int open_locale(char*path){
	int fd=-1,r=-1;
	struct stat st;
	if(locale_map){
		munmap(locale_map,map_size);
		locale_map=NULL;
		map_size=-1;
	}
	if((fd=open(path,O_RDONLY))<=0)goto clean;
	if(fstat(fd,&st)!=0)goto clean;
	if((map_size=st.st_size)<=0)goto clean;
	if(!(locale_map=mmap(NULL,map_size,PROT_READ,MAP_SHARED,fd,0))){
		map_size=-1;
		goto clean;
	}
	r=0;
	clean:
	if(fd>=0)close(fd);
	return r;
}
void simple_load_locale(const char*dir,const char*lang,const char*domain){
	if(!domain)return;
	char rl[64],path[PATH_MAX],*p;
	memset(rl,0,64);
	char*l=(char*)lang;
	if(!l)l=getenv("LC_ALL");
	if(!l)l=getenv("LANG");
	if(!l)l=getenv("LANGUAGE");
	if(!l)return;
	strncpy(rl,l,63);
	char*d=dir?(char*)dir:DEFAULT_LOCALE;
	for(;;){
		memset(path,0,PATH_MAX);
		snprintf(path,PATH_MAX-1,"%s/%s/LC_MESSAGES/%s.mo",d,rl,domain);
		if(open_locale(path)==0)break;
		if((p=strchr(rl,'.')))*p=0;
		else if((p=strchr(rl,'_')))*p=0;
		else break;
	}
}
char*simple_gettext(const char*msgid){
	if(!locale_map||!msgid)return (char*)msgid;
	char*ret=mo_lookup(locale_map,map_size,msgid);
	return ret?ret:(char*)msgid;
}
void init_locale(){
	simple_load_locale(NULL,NULL,NAME);
	init_commands_locale();
}
