#define _GNU_SOURCE
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdbool.h>
#include"defines.h"
#include"keyval.h"
#include"param.h"
#include"str.h"

static keyval**configs_add_entry(keyval**entrys,char*key,char*value,int idx){
	keyval**e;
	size_t size=sizeof(struct config_entry*)*(idx+1);
	if(!entrys){
		if(!(entrys=malloc(size)))return NULL;
		memset(entrys,0,size);
	}else if(!(e=realloc(entrys,size))){
		free(entrys);
		return NULL;
	}else entrys=e;
	if(!key)entrys[idx]=NULL;
	else{
		keyval*ent=kv_new_set(key,value);
		if(!ent){
			free(entrys);
			return NULL;
		}
		entrys[idx]=ent;
	}
	return entrys;
}

keyval**read_params(int fd){
	int eid=0;
	size_t idx=0;
	bool read_key=true;
	keyval**entrys=NULL;
	char*key=NULL,*value=NULL,*buff;
	char bit[2]={0,0},b,c;
	if(!(buff=malloc(PATH_MAX+1)))return NULL;
	memset(buff,0,PATH_MAX+1);
	while(read(fd,&bit,1)==1&&idx<PATH_MAX){
		b=bit[0];
		if(b=='"'||b=='\''){
			c=b;
			while(read(fd,&bit,1)==1&&idx<PATH_MAX){
				if(bit[0]==c)break;
				if(bit[0]=='\\'&&read(fd,&bit,1)!=1)continue;
				buff[idx++]=bit[0];
			}
		}else if(b=='='&&read_key){
			if(!(key=strndup(buff,idx)))goto e1;
			idx=0,read_key=false;
			memset(buff,0,PATH_MAX);
		}else if(b==' '||b=='\t'||b=='\n'||b=='\r'||b=='#'){
			if(idx==0)continue;
			if(!read_key&&!(value=strndup(buff,idx)))goto e1;
			idx=0,read_key=true;
			memset(buff,0,PATH_MAX);
			if(!(entrys=configs_add_entry(entrys,key,value,eid++)))goto e1;
			key=NULL,value=NULL;
			if(b=='#')skips(fd,"\r\n");
		}else buff[idx++]=b;
	}
	if(idx>0){
		if(!read_key&&!(value=strndup(buff,idx)))goto e1;
		if(!(entrys=configs_add_entry(entrys,key,value,eid++)))goto e1;
	}else if(key)free(key);
	free(buff);
	return configs_add_entry(entrys,NULL,NULL,eid);
	e1:
	if(key)free(key);
	if(value)free(value);
	free(buff);
	free(entrys);
	return NULL;
}

keyval**append_params(keyval**ptr,keyval**new){
	if(!new)return ptr;
	if(!ptr)return new;
	int idx=-1,ridx=0;
	while(ptr[++idx]);
	while(new[ridx]){
		ptr[++idx]=new[++ridx];
		if(!(ptr=realloc(ptr,sizeof(keyval*)*idx)))return NULL;
	}
	ptr[idx]=NULL;
	return ptr;
}
