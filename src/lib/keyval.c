#include<stdlib.h>
#include<string.h>
#include<stdio.h>
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
	if(!s)return;
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
		if(!(k=strndup(line,pos-line))||!(v=strdup(pos+1))){
			free(k);
			return NULL;
		}
		kv_set(kv,k,v);
	}else kv->key=line;
	return kv;
}

keyval*kv_new_set(char*key,char*value){
	return kv_set(kv_new(),key,value);
}

keyval*kv_new_set_dup(char*key,char*value){
	keyval*kv=kv_new();
	if(!kv||!key)return NULL;
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

keyval**kvarr_new_parse_arr(char**lines,char del){
	if(!lines)return NULL;
	size_t s=(char_array_len(lines)+1);
	return kvarr_parse_arr(kvarr_new(s),s,lines,del);
}

void kvarr_dump(keyval**kvs,char*del,char*ldel){
	if(!kvs)return;
	KVARR_FOREACH(kvs,kv,i){
		size_t s=1;
		s+=del?strlen(del):3;
		s+=kv->key?strlen(kv->key):6;
		s+=kv->value?strlen(kv->value):6;
		char*buff=malloc(s);
		if(!s)return;
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
	if(kvs&&key){
		keyval*kv=kvarr_get_by_key(kvs,key,NULL);
		if(kv)return kv->value;
	}
	return def;
}

char*kvarr_get_key_by_value(keyval**kvs,char*value,char*def){
	if(kvs&&value){
		keyval*kv=kvarr_get_by_value(kvs,value,NULL);
		if(kv)return kv->key;
	}
	return def;
}
