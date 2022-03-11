/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<time.h>
#include<ctype.h>
#include<errno.h>
#include<stdio.h>
#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<unistd.h>
#include"defines.h"
#include"list.h"
#include"array.h"
#include"str.h"

const char*size_units[]={"B","KiB","MiB","GiB","TiB","PiB","EiB","ZiB","YiB",NULL};

char*time2nstr(time_t*time,char*format,char*buff,size_t len){
	struct tm*timeinfo;
	timeinfo=localtime(time);
	strftime(buff,len,format,timeinfo);
	return buff;
}

char*time2str(time_t*time,char*format,char*buff){
	return time2nstr(time,format,buff,sizeof(buff));
}

char*time2defstr(time_t*time,char*buff){
	return time2str(time,_DEFAULT_TIME_FORMAT,buff);
}

char*time2ndefstr(time_t*time,char*buff,size_t len){
	return time2nstr(time,_DEFAULT_TIME_FORMAT,buff,len);
}

char*new_string(size_t size){
	char*a=malloc(size);
	if(!a)return NULL;
	memset(a,0,size);
	return a;
}

bool contains_of(const char*source,size_t size,char x){
	size_t idx=0;
	char c;
	if(!source)return false;
	while(idx<size&&(c=source[idx++])>0)if(c==x)return true;
	return false;
}

bool check_nvalid(const char*source,size_t size_source,const char*valid,size_t size_valid){
	size_t idx=0;
	char c;
	if(!source||!valid)return false;
	while(idx<size_source&&(c=source[idx++])>0)if(!contains_of(valid,size_valid,c))return false;
	return true;
}

bool check_valid(char*source,const char*valid){
	return check_nvalid(source,strlen(source),valid,strlen(valid));
}

bool check_nvalid_default(char*source,size_t size){
	return check_nvalid(source,size,VALID,sizeof(VALID));
}

bool check_valid_default(char*source){
	return check_nvalid_default(source,strlen(source));
}

char dec2hex(int dec,bool upper){
	if(dec>=0&&dec<=9)return (char)('0'+dec);
	else if(dec>=10&&dec<=15)return (char)((upper?'A':'a')+dec-10);
	else return 0;
}

char*gen_rand_hex(char*buff,int size,bool upper){
	if(size<0||!buff)return NULL;
	int idx=0;
	srand((unsigned)time(NULL)/size);
	while(idx<size)buff[idx++]=dec2hex(rand()%16,upper);
	buff[idx]=0;
	return buff;
}

char*strrep(char*str,char from,char to){
	if(!str)return NULL;
	size_t s=0;
	while(str[s]&&str[s]!=from)s++;
	if(str[s]==from)str[s]=to;
	return str;
}

#ifndef ENABLE_UEFI
int repeat(int fd,char c,size_t times){
	char*buff=malloc((times+1)*sizeof(char));
	if(!buff)return -errno;
	memset(buff,c,times);
	buff[times]=0;
	int r=dprintf(fd,"%s",buff);
	free(buff);
	fsync(fd);
	return r;
}
#endif

long parse_long(char*str,long def){
	if(!str)return def;
	errno=0;
	char*end;
	long val=strtol(str,&end,0);
	return errno!=0||end==str?def:val;
}

int parse_int(char*str,int def){
	return (int)parse_long(str,(int)def);
}

bool fuzzy_cmp(const char*s1,const char*s2){
	if(!s1&&!s2)return true;
	if(!s1||!s2)return false;
	size_t l1=strlen(s1),l2=strlen(s2);
	if(l1==0&&l2==0)return true;
	if(l1==0||l2==0)return false;
	return strncasecmp(s1,s2,min_int(l1,l2))==0;
}

bool fuzzy_cmps(const char*v,const char**s){
	if(!v&&!s)return true;
	if(!v||!s)return false;
	for(int i=0;(s[i]);i++)
		if(fuzzy_cmp(v,s[i]))return true;
	return false;
}

size_t possible_match(char*src,poss**p){
	if(!p||!src)return 0;
	ssize_t pending,matched=0;
	if((pending=strlen(src))<=0)return 0;
	for(size_t i=0,match=0;p[i];i++,match=0){
		if(
			!p[i]->data||
			p[i]->item_len<=0||p[i]->data_len<=0||p[i]->possible<=0||
			p[i]->possible*p[i]->item_len!=p[i]->data_len||
			p[i]->data_len%p[i]->item_len!=0
		)continue;
		pending-=p[i]->item_len;
		if(pending<0)return matched;
		for(
			size_t e=0,ce=0;
			e<p[i]->possible&&ce+p[i]->item_len<=p[i]->data_len;
			e++,ce=e*p[i]->item_len
		)if(strncmp(src+matched,p[i]->data+ce,p[i]->item_len)==0){
			match=p[i]->item_len;
			break;
		}
		if(match==0)return matched;
		matched+=match;
	}
	return matched;
}

size_t possible_length(poss**p){
	if(!p)return 0;
	size_t s,i;
	for(i=0,s=0;p[i];i++)s+=p[i]->item_len;
	return s;
}

bool check_identifier(char*str){
	if(!str)return false;
	if(!check_valid(str,VALID))return false;
	if(!contains_of(VALIDL,strlen(VALIDL),str[0]))return false;
	return true;
}

void trim(char*str){
	if(!str)return;
	size_t s=strlen(str);
	if(s<=0)return;
	char*start=str,*end=str+s-1;
	while(*start!=0&&start<end&&isspace(*start))start++;
	while(end>=start&&isspace(*end))end--;
	*(end+1)=0;
	if(start!=str){
		size_t x;
		for(x=0;start[x];x++)str[x]=start[x];
		str[x]=0;
	}
}

void strtoupper(char*str){
	if(!str)return;
	for(;*str;str++)*str=toupper(*str);
}

void strtolower(char*str){
	if(!str)return;
	for(;*str;str++)*str=tolower(*str);
}

void*_memdup(void*mem,size_t len){
	void*dup=malloc(len);
	if(!dup)EPRET(ENOMEM);
	memcpy(dup,mem,len);
	return dup;
}

list*path_simplify(list*paths,bool free){
	if(!paths)EPRET(EINVAL);
	if(!paths->next&&!paths->prev)return paths;
	list*cur=list_first(paths),*p;
	if(!cur)return NULL;
	for(;;){
		LIST_DATA_DECLARE(x,cur,char*);
		bool oo=strcmp(x,"..")==0;
		bool m=oo||strcmp(x,".")==0;
		if(m){
			if(oo&&cur->prev)list_remove_free(
				cur->prev,
				free?list_default_free:NULL
			);
			p=cur->next;
			if(!p)p=cur->prev;
			if(!p)return NULL;
			list_remove_free_def(cur);
			cur=p;
			continue;
		}
		if(!(cur->next))break;
		cur=cur->next;
	}
	return list_first(cur);
}

list*path2list(char*path,bool parent){
	if(!path)EPRET(EINVAL);
	size_t c=0;
	char*po=path;
	list*l=list_new_strdup(".");
	while(*path){
		if(*path!='/')c++;
		else if(c==0)po=path+1;
		else{
			if(list_push_new_strndup(l,po,c)<0)goto fail;
			po=path+1,c=0;
		}
		path++;
	}
	if((c>0&&list_push_new_strndup(l,po,c)<0)||!l->next)goto fail;
	l=l->next;
	list_remove_free_def(l->prev);
	errno=0;
	return parent?path_simplify(l,true):l;
	fail:
	list_free_all_def(l);
	return NULL;
}

char**path2array(char*path,bool parent){
	list*l=path2list(path,parent);
	if(!l)return NULL;
	char**a=list2array_chars(l);
	list_free_all(l,NULL);
	return a;
}

char*trim_slash(char*path){
	if(!path||!*path)return NULL;
	if(*path=='/')path++;
	size_t ps=strlen(path);
	if(path[ps-1]=='/')path[ps-1]=0;
	return *path?path:NULL;
}

char*add_right_slash(char*path,size_t len){
	if(!path||!*path)return NULL;
	size_t bs=strlen(path);
	if(bs>=len-1)return path;
	if(path[bs-1]!='/')path[bs]='/',path[bs+1]=0;
	return path;
}

char*buff2hex(char*hex,void*buff,size_t len){
	for(size_t i=0;i<len;i++)
		snprintf(hex+(i*2),3,"%02x",((unsigned char*)buff)[i]);
	return hex;
}

bool string_is_true(char*string){
	return
		strcasecmp(string,"1")==0||
		strcasecmp(string,"ok")==0||
		strcasecmp(string,"on")==0||
		strcasecmp(string,"yes")==0||
		strcasecmp(string,"true")==0||
		strcasecmp(string,"always")==0||
		strcasecmp(string,"enable")==0||
		strcasecmp(string,"enabled")==0;
}

bool string_is_false(char*string){
	return
		strcasecmp(string,"0")==0||
		strcasecmp(string,"no")==0||
		strcasecmp(string,"off")==0||
		strcasecmp(string,"false")==0||
		strcasecmp(string,"never")==0||
		strcasecmp(string,"disable")==0||
		strcasecmp(string,"disabled")==0;
}

char*str_escape(char*str){
	if(!str)EPRET(EINVAL);
	size_t xs=strlen(str)*2+1;
	char*dup=malloc(xs);
	if(!dup)EPRET(ENOMEM);
	memset(dup,0,xs);
	char*ptr_dup=dup,*ptr_str=str;
	while(*ptr_str){
		switch(*ptr_str){
			case '"':*ptr_dup++='\\',*ptr_dup++='\"';break;
			case '\\':*ptr_dup++='\\',*ptr_dup++='\\';break;
			case '\t':*ptr_dup++='\\',*ptr_dup++='t';break;
			case '\n':*ptr_dup++='\\',*ptr_dup++='n';break;
			case '\r':*ptr_dup++='\\',*ptr_dup++='r';break;
			default:*ptr_dup++=*ptr_str;
		}
		ptr_str++;
	}
	return dup;
}

char*str_unescape(char*str){
	if(!str)EPRET(EINVAL);
	size_t xs=strlen(str)+1;
	char*dup=malloc(xs);
	if(!dup)EPRET(ENOMEM);
	memset(dup,0,xs);
	char*ptr_dup=dup,*ptr_str=str;
	while(*ptr_str){
		switch(*ptr_str){
			case '\\':
				switch(*(++ptr_str)){
					case 't':*ptr_dup++='\t';break;
					case 'n':*ptr_dup++='\n';break;
					case 'r':*ptr_dup++='\r';break;
					default:*ptr_dup++=*ptr_str;break;
				}
			break;
			default:*ptr_dup++=*ptr_str;
		}
		ptr_str++;
	}
	return dup;
}

size_t _strlcpy(char *buf,const char*src,size_t len){
        char*d0=buf;
        if(!len--)goto finish;
        for(;len&&(*buf=*src);len--,src++,buf++);
        *buf=0;
	finish:
        return buf-d0+strlen(src);
}

size_t _strlcat(char*buf,const char*src,size_t len){
        size_t slen=strnlen(buf,len);
        return slen==len?
		(slen+strlen(src)):
		(slen+_strlcpy(buf+slen,src,len-slen));
}
