/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<signal.h>
#include"list.h"
#include"keyval.h"
#include"output.h"
#include"array.h"

void kv_free(keyval*kv){
	if(!kv)return;
	if(kv->key)free(kv->key);
	if(kv->value)free(kv->value);
	free(kv);
}

keyval*kv_init(keyval*kv){
	if(!kv)return NULL;
	memset(kv,0,sizeof(keyval));
	return kv;
}

long kv_get_long_value(keyval*kv,long def,int base){
	if(!kv||!kv->value)return def;
	errno=0;
	char*end;
	long l=strtol(kv->value,&end,base);
	return (*end||kv->value==end||errno!=0)?def:l;
}

keyval*kv_malloc(){
	return malloc(sizeof(keyval));
}

keyval*kv_xmalloc(){
	keyval*kv=kv_malloc();
	if(!kv){
		perror("allocate keyval");
		exit(1);
	}
	return kv;
}

keyval*kv_new(){
	return kv_init(kv_malloc());
}

char*kv_print(keyval*kv,char*buff,size_t bs,char*del){
	if(!kv||!buff)return NULL;
	snprintf(
		buff,bs,
		"%s%s%s",
		kv->key?kv->key:"(null)",
		del?del:" = ",
		kv->value?kv->value:"(null)"
	);
	return buff;
}

void kv_dump(keyval*kv,char*del){
	if(!kv)return;
	size_t s=1;
	s+=del?strlen(del):3;
	s+=kv->key?strlen(kv->key):6;
	s+=kv->value?strlen(kv->value):6;
	char*buff=malloc(s);
	if(!buff)return;
	memset(buff,0,s);
	kv_print(kv,buff,s,del);
	puts(buff);
	free(buff);
}

keyval*kv_set(keyval*kv,char*key,char*value){
	if(kv){
		kv->key=key;
		kv->value=value;
	}
	return kv;
}

keyval*kv_set_dup(keyval*kv,char*key,char*value){
	char*k,*v=NULL;
	if(!kv||!key)return NULL;
	if(!(k=strdup(key)))return NULL;
	if(value&&!(v=strdup(value))){
		free(k);
		return NULL;
	}
	return kv_set(kv,k,v);
}

keyval*kv_set_ndup(keyval*kv,char*key,size_t ks,char*value,size_t vs){
	char*k=NULL,*v=NULL;
	if(!kv||!key)return NULL;
	if(!(k=strndup(key,ks)))return NULL;
	if(value&&!(v=strndup(value,vs))){
		free(k);
		return NULL;
	}
	return kv_set(kv,k,v);
}

keyval*kv_parse(keyval*kv,char*line,char del){
	if(!kv||!line)return kv;
	kv_init(kv);
	char*pos=strchr(line,del),*k=NULL,*v=NULL;
	if(pos){
		if(
			!(k=strndup(line,pos-line))||
			!(v=strdup(pos+1))
		){
			if(k)free(k);
			if(v)free(v);
			return NULL;
		}
		kv_set(kv,k,v);
	}else kv->key=strdup(line);
	return kv;
}

keyval*kv_new_set(char*key,char*value){
	return kv_set(kv_new(),key,value);
}

keyval*kv_new_set_dup(char*key,char*value){
	if(!key)return NULL;
	keyval*kv=kv_new();
	if(!kv)return NULL;
	if(!kv_set_dup(kv,key,value)){
		kv_free(kv);
		return NULL;
	}
	return kv;
}

keyval*kv_new_set_ndup(char*key,size_t ks,char*value,size_t vs){
	keyval*kv=kv_new();
	if(!kv)return NULL;
	if(!kv_set_ndup(kv,key,ks,value,vs)){
		kv_free(kv);
		return NULL;
	}
	return kv;
}

keyval*kv_new_parse(char*line,char del){
	keyval*kv=kv_new();
	if(!kv)return NULL;
	if(!kv_parse(kv,line,del)){
		kv_free(kv);
		return NULL;
	}
	return kv;
}

void kvarr_free(keyval**kvs){
	if(!kvs)return;
	KVARR_FOREACH(kvs,kv,i)kv_free(kv);
	free(kvs);
}

keyval**kvarr_init(keyval**kvs,int count){
	if(!kvs)return NULL;
	memset(kvs,0,sizeof(keyval*)*count);
	return kvs;
}

keyval**kvarr_malloc(size_t len){
	return malloc(sizeof(keyval*)*len);
}

keyval**kvarr_xmalloc(size_t len){
	keyval**kvs=kvarr_malloc(len);
	if(!kvs){
		perror("allocate keyval array");
		exit(1);
	}
	return kvs;
}

keyval**kvarr_new(int len){
	return kvarr_init(kvarr_malloc(len),len);
}

keyval**kvarr_parse_arr(keyval**kvs,size_t s,char**lines,char del){
	if(!kvs||!lines)return NULL;
	char*line;
	for(size_t i=0;(line=lines[i])&&i<s;i++)
		kvs[i]=kv_new_parse(line,del);
	return kvs;
}

keyval**kvarr_parse(keyval**kvs,size_t s,char*lines,char ldel,char del){
	size_t i=0;
	char*d,*cur,*next;
	if(!kvs||!lines||!(cur=d=strdup(lines)))return NULL;
	do{
		if((next=strchr(cur,ldel)))next[0]=0,next++;
		kvs[i++]=kv_new_parse(cur,del);
	}while((cur=next)&&(s==0||i<s));
	free(d);
	return kvs;
}

keyval**kvarr_new_parse_arr(char**lines,char del){
	if(!lines)return NULL;
	size_t s=(char_array_len(lines)+1);
	keyval**k=kvarr_new(s);
	if(!k)return NULL;
	if(!kvarr_parse_arr(k,s,lines,del)){
		free(k);
		return NULL;
	}
	return k;
}

keyval**kvarr_new_parse(char*lines,char ldel,char del){
	if(!lines)return NULL;
	size_t s=0,x=strlen(lines);
	for(size_t i=0;i<x-1;i++)if(lines[i]==ldel)s++;
	if(lines[x]!=ldel)s++;
	keyval**k=kvarr_new(s+1);
	if(!k)return NULL;
	if(!kvarr_parse(k,s,lines,ldel,del)){
		free(k);
		return NULL;
	}
	return k;
}

void kvarr_dump(keyval**kvs,char*del,char*ldel){
	if(!kvs)return;
	KVARR_FOREACH(kvs,kv,i){
		size_t s=1;
		s+=del?strlen(del):3;
		s+=kv->key?strlen(kv->key):6;
		s+=kv->value?strlen(kv->value):6;
		char*buff=malloc(s);
		if(!buff)return;
		memset(buff,0,s);
		kv_print(kv,buff,s,del);
		printf("%s%s",buff,ldel?ldel:"\n");
		free(buff);
	}
}

size_t kvarr_count(keyval**kvs){
	if(!kvs)return 0;
	size_t i;
	for(i=0;kvs[i];i++);
	return i;
}

keyval*kvarr_get_by_key(keyval**kvs,char*key,keyval*def){
	if(key&&kvs){
		KVARR_FOREACH(kvs,item,i){
			if(!item->key)continue;
			if(strcmp(key,item->key)!=0)continue;
			return item;
		}
	}
	return def;
}

keyval*kvarr_multi_get_by_key(keyval***kvs,char*key,keyval*def){
	keyval*kv=NULL;
	if(key&&kvs)
		for(size_t s=0;kvs[s];s++)
			if((kv=kvarr_get_by_key(kvs[s],key,NULL)))
				return kv;
	return def;
}

keyval*kvarr_multi_get_by_value(keyval***kvs,char*value,keyval*def){
	keyval*kv=NULL;
	if(value&&kvs)
		for(size_t s=0;kvs[s];s++)
			if((kv=kvarr_get_by_value(kvs[s],value,NULL)))
				return kv;
	return def;
}

keyval*kvarr_get_by_value(keyval**kvs,char*value,keyval*def){
	if(value&&kvs){
		KVARR_FOREACH(kvs,item,i){
			if(!item->value)continue;
			if(strcmp(value,item->value)!=0)continue;
			return item;
		}
	}
	return def;
}

char*kvarr_get_value_by_key(keyval**kvs,char*key,char*def){
	keyval*kv=NULL;
	if(kvs&&key)
		if((kv=kvarr_get_by_key(kvs,key,NULL)))
			return kv->value;
	return def;
}

long kvarr_get_long_value_by_key(keyval**kvs,char*key,long def,int base){
	keyval*kv=NULL;
	if(kvs&&key)
		if((kv=kvarr_get_by_key(kvs,key,NULL)))
			return kv_get_long_value(kv,def,base);
	return def;
}

char*kvarr_get_key_by_value(keyval**kvs,char*value,char*def){
	keyval*kv=NULL;
	if(kvs&&value)
		if((kv=kvarr_get_by_value(kvs,value,NULL)))
			return kv->key;
	return def;
}

char*kvarr_multi_get_value_by_key(keyval***kvs,char*key,char*def){
	keyval*kv=NULL;
	if(kvs&&key&&(kv=kvarr_multi_get_by_key(kvs,key,NULL)))return kv->value;
	return def;
}

long kvarr_multi_get_long_value_by_key(keyval***kvs,char*key,long def,int base){
	keyval*kv=NULL;
	if(kvs&&key&&(kv=kvarr_multi_get_by_key(kvs,key,NULL)))
		return kv_get_long_value(kv,def,base);
	return def;
}

char*kvarr_multi_get_key_by_value(keyval***kvs,char*value,char*def){
	keyval*kv=NULL;
	if(kvs&&value&&(kv=kvarr_multi_get_by_value(kvs,value,NULL)))return kv->key;
	return def;
}

int _kv_free(void*d){
	kv_free(d);
	return 0;
}

void kvlst_free(list*kvs){
	if(!kvs)return;
	list_free_all(kvs,_kv_free);
}

list*kvlst_add_obj(list*kvs,keyval*obj){
	if(!obj||!obj->key)EPRET(EINVAL);
	list*h=list_new(obj);
	if(!h)EPRET(ENOMEM);
	if(kvs){
		list*cur,*next;
		if((next=list_first(kvs)))do{
			cur=next;
			if(!cur->data)continue;
			LIST_DATA_DECLARE(s,cur,keyval*);
			if(!s->key)continue;
			if(strcmp(s->key,obj->key)==0){
				free(h);
				EPRET(EEXIST);
			}
		}while((next=cur->next));
		if(list_push(kvs,h)<0){
			free(h);
			return NULL;
		}
	}else kvs=h;
	return kvs;
}

list*kvlst_add(list*kvs,char*key,char*value){
	return kvlst_add_obj(kvs,kv_new_set(key,value));
}

list*kvlst_set_obj(list*kvs,keyval*obj,bool _free){
	if(!obj||!obj->key)EPRET(EINVAL);
	list*cur,*next;
	if(kvs&&(next=list_first(kvs)))do{
		cur=next;
		if(!cur->data)continue;
		LIST_DATA_DECLARE(s,cur,keyval*);
		if(!s->key)continue;
		if(strcmp(s->key,obj->key)==0){
			char*v=obj->value;
			if(_free){
				free(obj->key);
				free(obj);
			}
			s->value=v;
			return kvs;
		}
	}while((next=cur->next));
	return kvlst_add_obj(kvs,obj);
}

list*kvlst_set(list*kvs,char*key,char*value){
	return kvlst_set_obj(kvs,kv_new_set_dup(key,value),true);
}

list*kvlst_del(list*kvs,char*key){
	if(!key||!kvs)EPRET(EINVAL);
	list*cur,*next;
	if((next=list_first(kvs)))do{
		cur=next;
		if(!cur->data)continue;
		LIST_DATA_DECLARE(s,cur,keyval*);
		if(strcmp(s->key,key)!=0)continue;
		if(list_is_alone(cur)){
			if(cur!=kvs)abort();
			kvlst_free(kvs);
			kvs=NULL;
		}else{
			if(cur==kvs){
				if(cur->prev)kvs=cur->prev;
				else if(cur->next)kvs=cur->next;
				else abort();
			}
			list_remove_free(cur,_kv_free);
		}
		break;
	}while((next=cur->next));
	return kvs;
}

list*kvlst_parse_arr(list*kvs,char**lines,char del){
	if(!lines)return NULL;
	char*line;
	for(size_t i=0;(line=lines[i]);i++)
		if(!(kvs=kvlst_set_obj(
			kvs,kv_new_parse(line,del),true
		)))return NULL;
	return kvs;
}

list*kvlst_parse(list*kvs,size_t s,char*lines,char ldel,char del){
	size_t i=0;
	char*d,*cur,*next;
	if(!lines||!(cur=d=strdup(lines)))return NULL;
	do{
		if((next=strchr(cur,ldel)))next[0]=0,next++;
		keyval*n=kv_new_parse(cur,del);
		if(!n||!(kvs=kvlst_set_obj(kvs,n,true))){
			free(d);
			if(n)kv_free(n);
			return NULL;
		}
	}while((cur=next)&&(s==0||i<s));
	free(d);
	return kvs;
}

void kvlst_dump(list*kvs,char*del,char*ldel){
	if(!kvs)return;
	list*cur,*next;
	if((next=list_first(kvs)))do{
		cur=next;
		if(!cur->data)continue;
		LIST_DATA_DECLARE(kv,cur,keyval*);
		size_t s=1;
		s+=del?strlen(del):3;
		s+=kv->key?strlen(kv->key):6;
		s+=kv->value?strlen(kv->value):6;
		char*buff=malloc(s);
		if(!buff)return;
		memset(buff,0,s);
		kv_print(kv,buff,s,del);
		printf("%s%s",buff,ldel?ldel:"\n");
		free(buff);
	}while((next=cur->next));
}

keyval*kvlst_get_by_key(list*kvs,char*key,keyval*def){
	if(!key||!kvs)return def;
	list*next,*cur;
	if((next=list_first(kvs)))do{
		cur=next;
		if(!cur->data)continue;
		LIST_DATA_DECLARE(s,cur,keyval*);
		if(!s->key)continue;
		if(strcmp(s->key,key)==0)return s;
	}while((next=cur->next));
	return def;
}

keyval*kvlst_get_by_value(list*kvs,char*value,keyval*def){
	if(!value||!kvs)return def;
	list*next,*cur;
	if((next=list_first(kvs)))do{
		cur=next;
		if(!cur->data)continue;
		LIST_DATA_DECLARE(s,cur,keyval*);
		if(!s->value)continue;
		if(strcmp(s->value,value)==0)return s;
	}while((next=cur->next));
	return def;
}

char*kvlst_get_value_by_key(list*kvs,char*key,char*def){
	if(!kvs||!key)return def;
	keyval*kv=kvlst_get_by_key(kvs,key,NULL);
	if(!kv)errno=ENOENT;
	return kv?kv->value:def;
}

char*kvlst_get_key_by_value(list*kvs,char*value,char*def){
	if(!kvs||!value)return def;
	keyval*kv=kvlst_get_by_value(kvs,value,NULL);
	if(!kv)errno=ENOENT;
	return kv?kv->key:def;
}
