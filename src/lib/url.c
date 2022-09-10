/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<string.h>
#include<stddef.h>
#include<stdbool.h>
#include"str.h"
#include"url.h"

#define NE(v)((v)&&*(v))

static char url_encoding_map[256];
static bool url_encoding_maps_initialized=false;

static void init_url_encoding_map(){
	unsigned char i;
	if(url_encoding_maps_initialized)return;
	memset(url_encoding_map,0,sizeof(url_encoding_map));
	for(i='a';i<='z';i++)url_encoding_map[i]=i;
	for(i='A';i<='Z';i++)url_encoding_map[i]=i;
	for(i='0';i<='9';i++)url_encoding_map[i]=i;
	url_encoding_map['-']='-';
	url_encoding_map['_']='_';
	url_encoding_map['.']='.';
	url_encoding_map['*']='*';
	url_encoding_maps_initialized=true;
}

static keyval*parse_single_query(const char*query,size_t len){
	char*p;
	keyval*kv;
	if(!query||!*query||len<=0)return NULL;
	p=memchr(query,'=',len);
	if(p==query||!(kv=kv_new()))return NULL;
	if(p){
		size_t kl=(size_t)(p-query),vl=len-kl-1;
		kv_set(kv,
			url_decode_alloc(query,kl),
			vl>0?url_decode_alloc(p+1,vl):NULL
		);
	}else kv_set(kv,url_decode_alloc(query,len),NULL);
	if(!kv->key){
		kv_free(kv);
		kv=NULL;
	}
	return kv;
}

static char*url_append_queries(
	char*buf,size_t len,
	int mode,list*lst,keyval**kvs
){
	size_t p=0;
	list*l=NULL;
	bool sep=false;
	if(!buf||len<=0)return NULL;
	switch(mode){
		case 1:if((l=list_first(lst)))break;//fallthrough
		case 2:if(kvs)break;//fallthrough
		default:return NULL;
	}
	for(;;){
		keyval*kv=NULL;
		switch(mode){
			case 1:
				if(!l)break;
				kv=LIST_DATA(l,keyval*),l=l->next;
			break;
			case 2:kv=kvs[p++];break;
		}
		if(!kv)break;
		if(sep)strlcat(buf,"&",len);
		if(kv->key){
			url_encode_append(kv->key,0,buf,len);
			sep=true;
		}
		if(kv->value){
			strlcat(buf,"=",len);
			url_encode_append(kv->value,0,buf,len);
			sep=true;
		}
	}
	return buf;
}

static char*url_append_schema(char*buf,size_t len,url*u){
	if(!buf||len<=0||!u)return NULL;
	if(NE(u->scheme))
		lsnprintf(buf,len,"%s:",u->scheme);
	strlcat(buf,"//",len);
	return buf;
}

static char*url_append_user_info(char*buf,size_t len,url*u){
	bool append=false;
	if(!buf||len<=0||!u)return NULL;
	if(NE(u->username))url_encode_append(
		u->username,0,buf,len
	),append=true;
	if(NE(u->password)){
		strlcat(buf,":",len);
		url_encode_append(
			u->password,0,buf,len
		);
		append=true;
	}
	if(append)strlcat(buf,"@",len);
	return append?buf:NULL;
}

static char*url_append_host(char*buf,size_t len,url*u){
	if(!buf||len<=0||!u||!NE(u->host))return NULL;
	if(strpbrk(u->host,"[]#?%"))
		url_encode_append(u->host,0,buf,len);
	else if(strpbrk(u->host,":/@"))
		lsnprintf(buf,len,"[%s]",u->host);
	else strlcat(buf,u->host,len);
	return buf;
}

static char*url_append_port(char*buf,size_t len,url*u){
	if(!buf||len<=0||!u||u->port<0)return NULL;
	lsnprintf(buf,len,":%d",u->port);
	return buf;
}

static char*url_append_authority(char*buf,size_t len,url*u){
	if(!buf||len<=0||!u)return NULL;
	bool success=false;
	if(url_append_user_info(buf,len,u))success=true;
	if(url_append_host(buf,len,u))success=true;
	if(url_append_port(buf,len,u))success=true;
	return success?buf:NULL;
}

static char*url_append_path(char*buf,size_t len,url*u){
	if(!buf||len<=0||!u||!NE(u->path))return NULL;
	if(u->path[0]!='/')strlcat(buf,"/",len);
	url_encode_append_skip(u->path,0,buf,len,"/");
	return buf;
}

static char*url_append_hierarchical(char*buf,size_t len,url*u){
	if(!buf||len<=0||!u)return NULL;
	bool success=false;
	if(url_append_authority(buf,len,u))success=true;
	if(url_append_path(buf,len,u))success=true;
	return success?buf:NULL;
}

static char*url_append_query(char*buf,size_t len,url*u){
	if(!buf||len<=0||!u||!NE(u->query))return NULL;
	if(NE(buf)&&!NE(u->path))strlcat(buf,"/",len);
	lsnprintf(buf,len,"?%s",u->query);
	return buf;
}

static char*url_append_fragment(char*buf,size_t len,url*u){
	if(!buf||len<=0||!u||!NE(u->fragment))return NULL;
	if(NE(buf)&&!NE(u->path)&&!NE(u->query))strlcat(buf,"/",len);
	lsnprintf(buf,len,"#%s",u->fragment);
	return buf;
}

static char*url_append_all(char*buf,size_t len,url*u){
	bool success=false;
	if(!buf||!u||len<=0)return NULL;
	if(url_append_schema(buf,len,u))success=true;
	if(url_append_hierarchical(buf,len,u))success=true;
	if(url_append_query(buf,len,u))success=true;
	if(url_append_fragment(buf,len,u))success=true;
	return success?buf:NULL;
}

static void url_parse_query(
	const char*url,size_t len,
	int mode,list**lst,keyval***kvs
){
	keyval*kv;
	char*c,*p,*sep;
	bool http=false;
	size_t l,s,pos=0,cnt;
	const char*del=":&";
	switch(mode){
		case 1:if(lst)break;//fallthrough
		case 2:if(kvs)break;//fallthrough
		default:return;
	}
	if(!url||!*url)return;
	s=len<=0?strlen(url):strnlen(url,len);
	sep=memchr(url,'?',s);
	if((c=memchr(url,':',s))){
		if(c[1]=='/'&&c[2]=='/')http=true;
		c=NULL;
	}
	if(!sep&&!http)p=(char*)url;
	else if(http&&!sep)return;
	else if(sep)p=sep+1,s-=sep-url+1;
	else return;
	if((c=memchr(p,'#',s)))s=c-p,c=NULL;
	cnt=strncnt(url,len,del)+1;
	if(mode==2&&!(*kvs=kvarr_new(cnt+1)))return;
	do{
		if(c)s-=c-p+1,p=c+1,c=NULL;
		for(l=0;del[l]&&!c;l++)c=memchr(p,del[l],s);
		if(c==p)continue;
		l=c?(size_t)(c-p):s;
		if((kv=parse_single_query(p,l))){
			if(mode==1)list_obj_add_new(lst,kv);
			if(mode==2)(*kvs)[pos]=kv;
			pos++;
		}
	}while(c&&pos<=cnt);
}

static size_t url_calc(url*u){
	if(!u)return 0;
	size_t len=0;
	if(u->query)len+=strlen(u->query);
	if(u->scheme)len+=strlen(u->scheme);
	if(u->username)len+=strlen(u->username);
	if(u->password)len+=strlen(u->password);
	if(u->fragment)len+=strlen(u->fragment);
	if(u->host)len+=strlen(u->host);
	if(u->path)len+=strlen(u->path);
	return len;
}

char*url_encode_map(const char*src,size_t src_len,char*out,size_t out_len,const char*map){
	if(!src||!out||out_len<4)return NULL;
	init_url_encoding_map();
	const char*m=map;
	size_t len=strlen(src),pos=0;
	if(!m)m=url_encoding_map;
	if(len>src_len&&src_len!=0)len=src_len;
	memset(out,0,out_len);
	for(size_t i=0;i<len&&pos<out_len-4;i++){
		unsigned char c=src[i];
		if(!m[c]){
			out[pos++]='%';
			out[pos++]=dec2hex(c>>4&0xF,true);
			out[pos++]=dec2hex(c&0xF,true);
		}else out[pos++]=m[c];
	}
	return out;
}

char*url_encode_skip(const char*src,size_t src_len,char*out,size_t out_len,const char*skip){
	char map[256];
	if(!src||!out||out_len<4||!skip)return NULL;
	init_url_encoding_map();
	memcpy(map,url_encoding_map,sizeof(map));
	for(int i=0,c;(c=skip[i]);i++)map[c]=c;
	return url_encode_map(src,src_len,out,out_len,map);
}

char*url_encode(const char*src,size_t src_len,char*out,size_t out_len){
	return url_encode_map(src,src_len,out,out_len,NULL);
}

char*url_encode_append_map(const char*src,size_t src_len,char*buf,size_t buf_len,const char*map){
	if(!src||!buf||buf_len<=0)return NULL;
	size_t l=strnlen(buf,buf_len);
	if(buf_len-1>l)url_encode_map(
		src,src_len,buf+l,
		buf_len-l-1,map
	);
	return buf;
}

char*url_encode_append_skip(const char*src,size_t src_len,char*buf,size_t buf_len,const char*skip){
	if(!src||!buf||buf_len<=0)return NULL;
	size_t l=strnlen(buf,buf_len);
	if(buf_len-1>l)url_encode_skip(
		src,src_len,buf+l,
		buf_len-l-1,skip
	);
	return buf;
}

char*url_encode_append(const char*src,size_t src_len,char*buf,size_t buf_len){
	return url_encode_append_map(src,src_len,buf,buf_len,NULL);
}

char*url_encode_alloc_map(const char*src,size_t src_len,const char*map){
	if(!src)return NULL;
	size_t len=strlen(src);
	if(len>src_len&&src_len!=0)len=src_len;
	size_t size=len*3+1;
	char*data=malloc(size);
	if(!data)return NULL;
	char*ret=url_encode_map(src,src_len,data,size,map);
	if(data!=ret)free(data),ret=NULL;
	return ret;
}

char*url_encode_alloc_skip(const char*src,size_t src_len,const char*skip){
	if(!src)return NULL;
	size_t len=strlen(src);
	if(len>src_len&&src_len!=0)len=src_len;
	size_t size=len*3+1;
	char*data=malloc(size);
	if(!data)return NULL;
	char*ret=url_encode_skip(src,src_len,data,size,skip);
	if(data!=ret)free(data),ret=NULL;
	return ret;
}

char*url_encode_alloc(const char*src,size_t src_len){
	return url_encode_alloc_map(src,src_len,NULL);
}

char*url_decode(const char*src,size_t src_len,char*out,size_t out_len){
	if(!src||!out||out_len<=0)return NULL;
	char x=0,c=0;
	size_t len=strlen(src),pos=0,i=0;
	if(len>src_len&&src_len!=0)len=src_len;
	memset(out,0,out_len);
	while(i<len&&pos<out_len-1){
		if(src[i]=='+')out[pos++]=' ',i++;
		else if(src[i]=='%'){
			i++;
			if((x=hex2dec(src[i]))==16)return NULL;
			c=x<<4,i++;
			if((x=hex2dec(src[i]))==16)return NULL;
			c|=x,i++;
			out[pos++]=c;
		}else out[pos++]=src[i++];
	}
	return out;
}

char*url_decode_alloc(const char*src,size_t src_len){
	size_t len=strlen(src);
	if(len>src_len&&src_len!=0)len=src_len;
	size_t size=len+1;
	char*data=malloc(size);
	if(!data)return NULL;
	char*ret=url_decode(src,src_len,data,size);
	if(data!=ret)free(data),ret=NULL;
	return ret;
}

list*url_parse_query_list(const char*url,size_t len){
	list*ret=NULL;
	url_parse_query(url,len,1,&ret,NULL);
	return ret;
}

keyval**url_parse_query_array(const char*url,size_t len){
	keyval**ret=NULL;
	url_parse_query(url,len,2,NULL,&ret);
	return ret;
}

char*url_generate_query_list(char*buf,size_t len,list*queries){
	return url_append_queries(buf,len,1,queries,NULL);
}

char*url_generate_query_list_alloc(list*queries){
	list*l;
	char*buf;
	size_t len=0;
	if((l=list_first(queries)))do{
		LIST_DATA_DECLARE(kv,l,keyval*);
		if(kv->key)len+=strlen(kv->key);
		if(kv->value)len+=strlen(kv->value);
		len+=16;
	}while((l=l->next));
	if(!(buf=malloc(len)))return NULL;
	memset(buf,0,len);
	return url_generate_query_list(buf,len,queries);
}

char*url_generate_query_array(char*buf,size_t len,keyval**queries){
	return url_append_queries(buf,len,2,NULL,queries);
}

char*url_generate_query_array_alloc(keyval**queries){
	char*buf;
	size_t len=0;
	KVARR_FOREACH(queries,kv,i){
		if(kv->key)len+=strlen(kv->key);
		if(kv->value)len+=strlen(kv->value);
		len+=16;
	}
	if(!(buf=malloc(len)))return NULL;
	memset(buf,0,len);
	return url_generate_query_array(buf,len,queries);
}

url*url_new(){
	url*u=malloc(sizeof(url));
	if(!u)return NULL;
	memset(u,0,sizeof(url));
	url_clean(u);
	return u;
}

bool url_parse(url*u,const char*url,size_t len){
	size_t s;
	char*c,*p,*full;
	if(!u||!url||!*url)return false;
	if(len<=0)len=strlen(url);
	if(!(full=strndup(url,len)))goto done;
	trim(full);
	p=full;
	url_clean(u);
	if((c=strstr(p,"://"))){
		if((s=c-p)>0&&!url_set_scheme(u,p,s))goto done;
		p=c+3;
	}
	if((c=strpbrk(p,"@/"))&&*c=='@'){
		char*info=p;
		size_t si=c-p;
		p=c+1;
		if((c=memchr(info,':',si))){
			s=c-info;
			if(!url_set_username(u,info,s))goto done;
			if(!url_set_password(u,c+1,p-c-2))goto done;
		}else if(!url_set_username(u,info,si))goto done;
	}
	if(*p=='['){
		if(!(c=strchr(p,']')))goto done;
		if(!url_set_host(u,p+1,c-p-1))goto done;
		p=c+1;
	}else if(*p!='/'&&*p!=':'&&*p!='?'&&*p!='#'){
		c=strpbrk(p,":/?#"),s=c?(size_t)(c-p):0;
		if(!url_set_host(u,p,s))goto done;
		if(!c)goto success;
		p=c;
	}
	if(*p==':'){
		p++,c=strpbrk(p,"/?#"),s=c?(size_t)(c-p):0;
		if(c!=p){
			char*e=NULL;
			errno=0,u->port=strtol(p,&e,0);
			if(errno!=0||(*e&&e!=c))goto done;
			if(!c)goto success;
			p=c;
		}
	}
	if(*p=='/'){
		c=strpbrk(p,"?#");
		if(!url_set_path(u,p,c?c-p:0))goto done;
		if(!c)goto success;
		p=c;
	}
	if(*p=='?'){
		p++,c=strchr(p,'#');
		if(c!=p)url_set_query(u,p,c?c-p:0);
		if(!c)goto success;
		p=c;
	}
	if(*p=='#'&&!url_set_fragment(u,p+1,0))goto done;
	success:
	free(full);
	return true;
	done:
	if(full)free(full);
	url_clean(u);
	return false;
}

url*url_parse_relative_path(url*u,url*n,const char*path,size_t len){
	size_t s=8,k;
	bool ls=false;
	char*c,*p,*buf;
	list*l=NULL,*lp=NULL,*lx;
	if(!u||!path||!*path)return NULL;
	if(len<=0)len=strlen(path);
	if(!(buf=strndup(path,len)))goto done;
	trim(buf);
	p=buf;
	if((c=strstr(buf,"://"))){
		if(!url_parse(n,path,len))goto done;
		free(buf);
		return n;
	}
	if((c=strpbrk(p,"?#")))switch(*c){
		case '?':{
			*c=0,p=c+1,c=strchr(p,'#');
			if(c!=p)url_set_query(n,p,c?c-p:0);
		}//fallthrough
		case '#':{
			*c=0,p=c+1;
			url_set_fragment(n,p,0);
		}
	}
	trim_path(buf);
	k=strlen(buf);
	if(buf[k-1]=='/')ls=true,buf[k--]=0;
	s+=k;
	lp=path2list(buf,false);
	if(u->path&&buf[0]!='/'){
		free(buf);
		if(!(buf=strdup(u->path)))goto done;
		k=strlen(buf);
		while(buf[k-1]!='/')buf[--k]=0;
		l=path2list(buf,true);
		free(buf);
		s+=k,buf=NULL;
		list_push(l,lp);
	}
	if(!l)l=lp;
	lp=NULL;
	lx=path_simplify(l,true);
	l=lx;
	if(n->path)free(n->path);
	if(!(n->path=malloc(s)))goto done;
	memset(n->path,0,s);
	list_string_append(l,n->path,s,"/");
	trim_path(n->path);
	if(ls||!n->path[0])strlcat(n->path,"/",s);
	list_free_all_def(l);
	return n;
	done:
	if(buf)free(buf);
	if(l)list_free_all_def(l);
	if(lp)list_free_all_def(lp);
	return NULL;
}

url*url_parse_new(const char*url,size_t len){
	struct url*u=url_new();
	if(!u)return NULL;
	if(url_parse(u,url,len))return u;
	url_free(u);
	return NULL;
}

void url_clean(url*u){
	if(!u)return;
	if(u->query)free(u->query);
	if(u->scheme)free(u->scheme);
	if(u->username)free(u->username);
	if(u->password)free(u->password);
	if(u->fragment)free(u->fragment);
	if(u->host)free(u->host);
	if(u->path)free(u->path);
	memset(u,0,sizeof(url));
	u->port=-1;
}

url*url_copy(url*old,url*new){
	if(!old||!new)return NULL;
	url_clean(new);
	if(old->query&&!(new->query=strdup(old->query)))goto fail;
	if(old->scheme&&!(new->scheme=strdup(old->scheme)))goto fail;
	if(old->username&&!(new->username=strdup(old->username)))goto fail;
	if(old->password&&!(new->password=strdup(old->password)))goto fail;
	if(old->fragment&&!(new->fragment=strdup(old->fragment)))goto fail;
	if(old->host&&!(new->host=strdup(old->host)))goto fail;
	if(old->path&&!(new->path=strdup(old->path)))goto fail;
	new->port=old->port;
	return new;
	fail:url_clean(new);
	return NULL;
}

url*url_dup(url*u){
	if(!u)return NULL;
	url*n=url_new();
	if(!n)return NULL;
	url_copy(u,n);
	return n;
}

bool url_equals(url*u1,url*u2){
	if(u1==u2)return true;
	if(!u1||!u2)return false;
	if(memcmp(u1,u2,sizeof(url))==0)return true;
	if(u1->port!=u2->port)return false;
	if((u1->host==NULL)!=(u2->host==NULL))return false;
	if((u1->path==NULL)!=(u2->path==NULL))return false;
	if((u1->query==NULL)!=(u2->query==NULL))return false;
	if((u1->scheme==NULL)!=(u2->scheme==NULL))return false;
	if((u1->fragment==NULL)!=(u2->fragment==NULL))return false;
	if((u1->username==NULL)!=(u2->username==NULL))return false;
	if((u1->password==NULL)!=(u2->password==NULL))return false;
	if(u1->host&&strcmp(u1->host,u2->host)!=0)return false;
	if(u1->path&&strcmp(u1->path,u2->path)!=0)return false;
	if(u1->query&&strcmp(u1->query,u2->query)!=0)return false;
	if(u1->scheme&&strcmp(u1->scheme,u2->scheme)!=0)return false;
	if(u1->fragment&&strcmp(u1->fragment,u2->fragment)!=0)return false;
	if(u1->username&&strcmp(u1->username,u2->username)!=0)return false;
	if(u1->password&&strcmp(u1->password,u2->password)!=0)return false;
	return true;
}

bool url_go_back(url*u,bool clean){
	size_t len;
	bool s=false,ch=false;
	if(!u||!u->path)return false;
	if((len=strlen(u->path))<=0)return false;
	if(strcmp(u->path,"/")==0)return false;
	if(u->path[len-1]=='/')u->path[--len]=0,s=true,ch=true;
	while(len>0&&u->path[len-1]!='/')u->path[--len]=0,ch=true;
	if(ch&&!s&&len>0&&u->path[len-1]=='/')u->path[--len]=0;
	if(ch&&clean){
		url_set_query(u,NULL,0);
		url_set_fragment(u,NULL,0);
	}
	return ch;
}

bool url_is_on_top(url*u){
	if(!u)return false;
	if(!u->path||!u->path[0])return true;
	if(u->path[0]=='/'&&!u->path[1])return true;
	return false;
}

void url_free(url*u){
	if(!u)return;
	url_clean(u);
	free(u);
}

char*url_generate(char*buf,size_t len,url*u){
	if(!buf||len<=0||!u)return NULL;
	memset(buf,0,len);
	return url_append_all(buf,len,u);
}

char*url_generate_alloc(url*u){
	if(!u)return NULL;
	char*buffer=NULL;
	size_t l=0;
	if(l<=0)l=url_calc(u)*4+64;
	if(l<=0)l=8192;
	if(!(buffer=malloc(l)))return NULL;
	char*ret=url_generate(buffer,l,u);
	if(ret)return ret;
	free(buffer);
	return NULL;
}

char*url_generate_authority(char*buf,size_t len,url*u){
	if(!buf||len<=0||!u)return NULL;
	memset(buf,0,len);
	return url_append_authority(buf,len,u);
}

void url_dump(char*buf,size_t len,url*u){
	if(!u||!buf||len<=0)return;
	memset(buf,0,len);
	lsnprintf(buf,len,"dump of url %p:\n",u);
	if(u->scheme)lsnprintf(buf,len,"  scheme: \"%s\"\n",u->scheme);
	if(u->username)lsnprintf(buf,len,"  username: \"%s\"\n",u->username);
	if(u->password)lsnprintf(buf,len,"  password: \"%s\"\n",u->password);
	if(u->host)lsnprintf(buf,len,"  host: \"%s\"\n",u->host);
	if(u->port>=0)lsnprintf(buf,len,"  port: %d\n",u->port);
	if(u->path)lsnprintf(buf,len,"  path: \"%s\"\n",u->path);
	if(u->query)lsnprintf(buf,len,"  query: \"%s\"\n",u->query);
	if(u->fragment)lsnprintf(buf,len,"  fragment: \"%s\"\n",u->fragment);
}

char*url_dump_alloc(url*u){
	if(!u)return NULL;
	char*buffer=NULL;
	size_t len=8192+(url_calc(u)*4);
	if((buffer=malloc(len)))
		url_dump(buffer,len,u);
	return buffer;
}

#ifndef ENABLE_UEFI
void url_dump_fd(int fd,url*u){
	char*buffer=url_dump_alloc(u);
	if(!buffer)return;
	dprintf(fd,"%s",buffer);
	free(buffer);
}
#endif

char*url_set_query_list(url*u,list*queries){
	if(!u||!queries)return NULL;
	if(u->query)free(u->query);
	u->query=url_generate_query_list_alloc(queries);
	return u->query;
}

char*url_set_query_array(url*u,keyval**queries){
	if(!u||!queries)return NULL;
	if(u->query)free(u->query);
	u->query=url_generate_query_array_alloc(queries);
	return u->query;
}

list*url_get_query_list(url*u){
	if(!u||!u->query)return NULL;
	return url_parse_query_list(u->query,0);
}

keyval**url_get_query_array(url*u){
	if(!u||!u->query)return NULL;
	return url_parse_query_array(u->query,0);
}

#define DECL_SET(_name,_field,_alloc,_expr...) \
        char*url_set_##_name(url*u,const char*val,size_t len){\
		if(!u)return NULL;\
		if(u->_field)free(u->_field);\
		u->_field=NULL;\
		if(!val||!*val)return NULL;\
		if(len==0)len=strlen(val);\
		_expr;\
		if(!u->_field)u->_field=_alloc(val,len);\
		return u->_field;\
	}
#define DECL_SET_RAW(_name,_field) \
	DECL_SET(_name,_field,strndup,)
#define DECL_SET_DECODE(_field) \
	DECL_SET_RAW(_field##_decoded,_field) \
	DECL_SET(_field,_field,url_decode_alloc,)

DECL_SET_RAW(scheme,scheme)
DECL_SET_DECODE(username)
DECL_SET_DECODE(password)
DECL_SET_DECODE(host)
DECL_SET_RAW(query,query)
DECL_SET_RAW(fragment,fragment)
DECL_SET_RAW(path_decoded,path)
DECL_SET(path,path,strndup,if(memchr(val,'%',len))u->path=url_decode_alloc(val,len))
