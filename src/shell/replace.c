#define _GNU_SOURCE
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<sys/utsname.h>
#include"shell_internal.h"
#include"defines.h"
#include"system.h"
#include"str.h"

static void _append(size_t*cur,char*dest,char*str,size_t size){
	if(!cur||!str||!dest)return;
	strncpy(dest+*cur,str,size-*cur);
	*cur+=strlen(str);
}

static inline void _fappend(size_t*cur,char*dest,char*str,size_t size){
	_append(cur,dest,str,size);
	free(str);
}

#define xappend(str) _append(&dc,dest,str,size)
#define xfappend(str) _fappend(&dc,dest,str,size)

char*shell_replace(char*dest,char*src,size_t size){
	if(!src||!dest)return NULL;
	memset(dest,0,size);
	uid_t u=geteuid();
	size_t sc=0,dc=0;
	char prev=0,n[PATH_MAX],sv[1024],user[128];
	struct utsname uts;
	memset(user,0,128);
	memset(sv,0,1024);
	memset(n,0,PATH_MAX);
	strncpy(sv,src,1023);
	getcwd(n,PATH_MAX-1);
	uname(&uts);
	for(;;){
		if(sv[sc]==0||size<=dc)break;
		if(prev=='\\'){
			switch(sv[sc]){
				case '[':case ']':break;
				case '\\':dest[dc++]='\\';sv[sc]=' ';break;
				case 'e':dest[dc++]='\033';break;
				case '$':dest[dc++]=u==0?'#':'$';break;
				case 'u':xappend(get_username(u,user,127));break;
				case 'w':xappend(n);break;
				case 'W':xappend(basename(n));break;
				case 'h':xappend(uts.nodename);break;
				case 'H':xfappend(strrep(strdup(uts.nodename),'.',0));break;
				case 'v':case 'V':if(exit_code!=0){
					char code[8]={0};
					snprintf(code,7,"%d%c",exit_code,sv[sc]=='V'?':':0);
					xappend(code);
				}break;
				default:xappend(((char[]){'\\',sv[sc],0}));break;
			}
		}else if(sv[sc]!='\\')dest[dc++]=sv[sc];
		prev=sv[sc];
		sc++;
	}
	return dest;
}
