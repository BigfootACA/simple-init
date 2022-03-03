#define _GNU_SOURCE
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdarg.h>
#include<stdbool.h>
#include"defines.h"
#include"proctitle.h"

static struct{
	char*arg0;
	char*base,*end;
	char*nul;
	bool reset;
	int error;
}SPT;

int spt_copyenv(char*oldenv[]){
	char*eq;
	int i,error;
	if(environ!=oldenv)return 0;
	if((error=clearenv()))goto error;
	for(i=0;oldenv[i];i++){
		if(!(eq=strchr(oldenv[i],'=')))continue;
		*eq='\0';
		error=(0!=setenv(oldenv[i],eq+1,1))?errno:0;
		*eq='=';
		if(error)goto error;
	}
	return error;
	error:environ=oldenv;
	return error;
}

int spt_copyargs(int argc,char*argv[]){
	char*tmp;
	int i;
	for(i=1;i<argc||(i>=argc&&argv[i]);i++){
		if(!argv[i])continue;
		size_t s=sizeof(char)*(strlen(argv[i])+1);
		if(!(tmp=malloc(s)))return errno;
		memset(tmp,0,s);
		strcpy(tmp,argv[i]);
		argv[i]=tmp;
	}
	return 0;
}

void spt_init(int argc,char*argv[]){
	if(!argv||!argv[0])return;
	char**envp=environ;
	char*base,*end,*nul;
	int i,error;
	if(!(base=argv[0]))return;
	nul=&base[strlen(base)];
	end=nul+1;
	for(i=0;i<argc||(i>=argc&&argv[i]);i++){
		if(!argv[i]||argv[i]<end)continue;
		end=argv[i]+strlen(argv[i])+1;
	}
	for(i=0;envp[i];i++){
		if(envp[i]<end)continue;
		end=envp[i]+strlen(envp[i])+1;
	}
	if(!argv[i])return;
	size_t s=sizeof(char)*PATH_MAX;
	if(!(SPT.arg0=malloc(s)))goto syerr;
	memset(SPT.arg0,0,s);
	strcpy(SPT.arg0,argv[i]);
	if((error=spt_copyenv(envp)))goto error;
	if((error=spt_copyargs(argc,argv)))goto error;
	SPT.nul=nul;
	SPT.base=base;
	SPT.end=end;
	return;
	syerr:error=errno;
	error:SPT.error=error;
}

void setproctitle(const char*fmt,...){
	char buf[1024];
	va_list ap;
	char*nul;
	int len,error;
	if(!SPT.base)return;
	if(fmt){
		va_start(ap,fmt);
		len=vsnprintf(buf,sizeof(buf),fmt,ap);
		va_end(ap);
	}else len=snprintf(buf,sizeof(buf),"%s",SPT.arg0);
	if(len<=0){
		error=errno;
		goto error;
	}
	if(!SPT.reset){
		memset(SPT.base,0,SPT.end-SPT.base);
		SPT.reset=1;
	}else memset(SPT.base,0,min_int(sizeof(buf),(size_t)(SPT.end-SPT.base)));
	len=min_int(len,min_int((int)sizeof(buf),(int)(SPT.end-SPT.base)-1));
	memcpy(SPT.base,buf,len);
	nul=&SPT.base[len];
	if(nul<SPT.nul)*SPT.nul=0;
	else if(nul==SPT.nul&&&nul[1]<SPT.end){
		*SPT.nul=' ';
		*++nul='\0';
	}
	return;
	error:SPT.error=error;
}
