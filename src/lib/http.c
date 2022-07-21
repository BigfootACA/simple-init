/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_MICROHTTPD
#include<time.h>
#include<zlib.h>
#include<stddef.h>
#include<string.h>
#include<stdbool.h>
#include<microhttpd.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include"regexp.h"
#include"assets.h"
#include"logger.h"
#include"http.h"
#include"str.h"
#define TAG "http"
#define TIME_FMT "%a, %d %b %Y %H:%M:%S GMT"

struct fd_data{
	int fd;
	bool auto_close;
	void*data;
	size_t length;
};

enum MHD_Result http_ret_code_headers(
	struct http_hand_info*i,
	int code,
	keyval**kvs
){
	struct MHD_Response*r;
	if(!i||code<100||code>=600)return MHD_NO;
	r=MHD_create_response_from_buffer(0,NULL,MHD_RESPMEM_PERSISTENT);
	if(kvs)for(size_t c=0;kvs[c];c++)
		MHD_add_response_header(r,kvs[c]->key,kvs[c]->value);
	enum MHD_Result x=MHD_queue_response(i->conn,code,r);
	MHD_destroy_response(r);
	return x;
}

enum MHD_Result http_ret_code(
	struct http_hand_info*i,
	int code
){
	return http_ret_code_headers(i,code,NULL);
}

enum MHD_Result http_ret_redirect(
	struct http_hand_info*i,
	int code,
	const char*path
){
	keyval loc=KV(MHD_HTTP_HEADER_LOCATION,(char*)path);
	keyval*kvs[]={&loc,NULL};
	return http_ret_code_headers(i,code,kvs);
}

enum MHD_Result http_hand_code(struct http_hand_info*i){
	if(!i||!i->hand)return MHD_NO;
	return http_ret_code(i,i->hand->spec.code);
}

enum MHD_Result http_check_last_modify(
	struct http_hand_info*i,
	time_t time
){
	struct tm tm;
	const char*val;
	if(!i)return MHD_NO;
	if(!(val=MHD_lookup_connection_value(
		i->conn,MHD_HEADER_KIND,
		MHD_HTTP_HEADER_IF_MODIFIED_SINCE
	)))return MHD_NO;
	memset(&tm,0,sizeof(tm));
	strptime(val,TIME_FMT,&tm);
	time_t t=mktime(&tm)+tm.tm_gmtoff;
	if(t>=time)return http_ret_code(i,MHD_HTTP_NOT_MODIFIED);
	return MHD_NO;
}

bool http_parse_range(
	struct http_hand_info*i,
	int*code,
	size_t len,
	size_t*start,
	size_t*end
){
	const char*val;
	char buf[128],*bp=buf,*p=NULL;
	char*ss=NULL,*se=NULL,*xe=NULL;
	if(!i||!start||!end||!code)return false;
	*code=MHD_HTTP_OK;
	if(!(val=MHD_lookup_connection_value(
		i->conn,MHD_HEADER_KIND,
		MHD_HTTP_HEADER_RANGE
	)))return false;
	memset(bp,0,sizeof(buf));
	strncpy(bp,val,sizeof(buf)-1);
	trim(bp);
	if(strncasecmp(bp,"bytes",5)!=0)
		return false;
	bp+=5,trim(bp);
	if(*bp!='=')return false;
	if(strchr(bp,','))return false;
	bp++,trim(bp);
	if(!(p=strchr(bp,'-')))return false;
	*p=0,ss=bp,se=p+1;
	trim(ss),trim(se),errno=0;
	if(!*ss&&!*se)return false;
	if(*ss){
		*start=strtoll(ss,&xe,0);
		if(errno!=0||xe==ss)return false;
	}else *start=0;
	if(*se){
		*end=strtoll(se,&xe,0);
		if(errno!=0||xe==se)return false;
		if(*end>=len)*end=len-1;
	}else *end=len-1;
	if(*start>=len||*start>*end){
		*code=MHD_HTTP_RANGE_NOT_SATISFIABLE;
		return false;
	}
	*code=MHD_HTTP_PARTIAL_CONTENT;
	return true;
}

void http_add_range(
	struct MHD_Response*r,
	int code,
	size_t start,
	size_t end,
	size_t len
){
	char buf[128];
	memset(buf,0,sizeof(buf));
	if(end>=len)end=len-1;
	if(start>end)start=end;
	switch(code){
		case MHD_HTTP_RANGE_NOT_SATISFIABLE:
			snprintf(
				buf,sizeof(buf)-1,
				"bytes */%zu",len
			);
		break;
		case MHD_HTTP_PARTIAL_CONTENT:
			snprintf(
				buf,sizeof(buf)-1,
				"bytes %zu-%zu/%zu",
				start,end,len
			);
		break;
		case MHD_HTTP_OK:MHD_add_response_header(
			r,MHD_HTTP_HEADER_ACCEPT_RANGES,"bytes"
		);return;
	}
	MHD_add_response_header(
		r,MHD_HTTP_HEADER_CONTENT_RANGE,buf
	);
}

list*http_get_encodings(struct http_hand_info*i){
	const char*val;
	list*items=NULL;
	char buf[256],*p=NULL,*xb=buf;
	if(!(val=MHD_lookup_connection_value(
		i->conn,MHD_HEADER_KIND,
		MHD_HTTP_HEADER_ACCEPT_ENCODING
	)))return NULL;
	memset(buf,0,sizeof(buf));
	strncpy(buf,val,sizeof(buf)-1);
	while(*xb){
		if((p=strchr(xb,',')))*p=0;
		trim(xb);
		if(*xb)list_obj_add_new_strdup(&items,xb);
		if(p)xb=p+1;
		else break;
	}
	return items;
}

bool http_has_deflate(struct http_hand_info*i){
	list*l=http_get_encodings(i);
	bool ret=list_search_case_string(l,"deflate")!=NULL;
	list_free_all_def(l);
	return ret;
}

bool http_check_can_deflate(
	struct http_hand_info*i,
	size_t len,
	const char*mime
){
	if(len<1024||len>32*1024*1024)return false;
	if(!http_has_deflate(i))return false;
	if(strncasecmp(mime,"text/",5)==0)return true;
	if(strcasecmp(mime,"image/bmp")==0)return true;
	if(strcasecmp(mime,"application/json")==0)return true;
	if(strcasecmp(mime,"application/javascript")==0)return true;
	if(strcasecmp(mime+strlen(mime)-4,"+xml")==0)return true;
	return false;
}

struct MHD_Response*http_create_zlib_response(
	void*buf,size_t len,
	enum MHD_ResponseMemoryMode m
){
	struct MHD_Response*r=NULL;
	uLongf zlen=compressBound(len);
	void*zbuf=malloc(zlen);
	if(!zbuf)return NULL;
	int x=compress(zbuf,&zlen,buf,len);
	if(x!=Z_OK){
		tlog_warn("compress response failed: %d",x);
		free(zbuf);
		return NULL;
	}
	r=MHD_create_response_from_buffer(
		zlen,zbuf,MHD_RESPMEM_MUST_FREE
	);
	MHD_add_response_header(
		r,MHD_HTTP_HEADER_CONTENT_ENCODING,
		"deflate"
	);
	if(m==MHD_RESPMEM_MUST_FREE)free(buf);
	return r;
}

void http_add_time_header(
	struct MHD_Response*r,
	const char*key,
	time_t t
){
	char buf[128];
	if(!r||!key)return;
	memset(buf,0,sizeof(buf));
	strftime(buf,sizeof(buf)-1,TIME_FMT,gmtime(&t));
	MHD_add_response_header(r,key,buf);
}

void http_add_file_name_header(
	struct MHD_Response*r,
	const char*name
){
	char buf[512],fn[256];
	if(!r||!name)return;
	memset(fn,0,sizeof(fn));
	memset(buf,0,sizeof(buf));
	strncpy(fn,name,sizeof(fn)-1);
	snprintf(
		buf,sizeof(buf)-1,
		"inline; filename=\"%s\"",
		basename(name)
	);
	MHD_add_response_header(
		r,
		MHD_HTTP_HEADER_CONTENT_DISPOSITION,
		buf
	);
}

enum MHD_Result http_ret_assets_file(
	struct http_hand_info*i,
	entry_file*file
){
	time_t mt;
	char mime[128];
	void*buffer=NULL;
	int code=MHD_HTTP_OK;
	size_t len=0,start=0,end=0;
	struct MHD_Response*r=NULL;
	if(!i||!file)return MHD_NO;
	mt=file->info.mtime.tv_sec;
	if(http_check_last_modify(i,mt)==MHD_YES)
		return MHD_YES;
	buffer=file->content,len=file->length,end=len-1;
	mime_get_by_filename(mime,sizeof(mime),file->info.name);
	if(http_parse_range(i,&code,file->length,&start,&end))
		len=end+1-start,buffer+=start;
	if(code!=MHD_HTTP_OK&&code!=MHD_HTTP_PARTIAL_CONTENT)
		len=0,buffer=NULL;
	if(
		buffer&&http_check_can_deflate(i,len,mime)
	)r=http_create_zlib_response(
		buffer,len,MHD_RESPMEM_PERSISTENT
	);
	if(!r)r=MHD_create_response_from_buffer(
		len,buffer,MHD_RESPMEM_PERSISTENT
	);
	http_add_range(r,code,start,end,file->length);
	if(code==MHD_HTTP_OK)http_add_time_header(
		r,MHD_HTTP_HEADER_LAST_MODIFIED,mt
	);
	if(buffer){
		http_add_file_name_header(r,file->info.name);
		MHD_add_response_header(
			r,MHD_HTTP_HEADER_CONTENT_TYPE,mime
		);
	}
	enum MHD_Result x=MHD_queue_response(i->conn,code,r);
	MHD_destroy_response(r);
	return x;
}

enum MHD_Result http_ret_assets_folder(
	struct http_hand_info*i,
	entry_dir*root,
	const char**index
){
	entry_file*file;
	char url[PATH_MAX],*u=(char*)i->url;
	if(!i||!root)return MHD_NO;
	if(strstr(u,"/../"))return MHD_NO;
	if(strcmp(u,"/..")==0)return MHD_NO;
	if(strcmp(u,"../")==0)return MHD_NO;
	if(strcmp(u,"..")==0)return MHD_NO;
	while(u[0]=='/')u++;
	strncpy(url,u,sizeof(url)-1);
	strncpy(url,u,sizeof(i->url)-1);
	if(!(file=get_assets_file(root,url))){
		entry_dir*d=get_assets_dir(root,url);
		if(!d){
			telog_verbose("%s not found",url);
			return MHD_NO;
		}
		if(strlen(i->url)>1&&url[strlen(url)-1]!='/'){
			strlcat(url,"/",sizeof(url)-1);
			return http_ret_redirect(i,MHD_HTTP_MOVED_PERMANENTLY,url);
		}
		if(index)for(int x=0;index[x];x++)
			if((file=get_assets_file(d,index[x])))break;
	}
	if(!file||!file->content)return MHD_NO;
	if(asset_file_check_out_bound(root,file))return MHD_NO;
	return http_ret_assets_file(i,file);
}

enum MHD_Result http_hand_assets_file(struct http_hand_info*i){
	entry_file*file;
	if(!i||!i->hand)return MHD_NO;
	if(!(file=i->hand->spec.assets_file.by_file.file)){
		entry_dir*dir=i->hand->spec.assets_file.by_path.dir;
		if(!dir)dir=&assets_rootfs;
		file=get_assets_file(dir,i->hand->spec.assets_file.by_path.path);
		if(file&&asset_file_check_out_bound(dir,file))return MHD_NO;
	}
	if(!file||!file->content)return MHD_NO;
	return http_ret_assets_file(i,file);
}

enum MHD_Result http_hand_assets(struct http_hand_info*i){
	entry_dir*dir;
	const char**index;
	if(!i||!i->hand)return MHD_NO;
	dir=i->hand->spec.assets.dir;
	index=i->hand->spec.assets.index;
	if(!dir)dir=&assets_rootfs;
	if(i->hand->spec.assets.path)
		dir=get_assets_dir(dir,i->hand->spec.assets.path);
	return http_ret_assets_folder(i,dir,index);
}

static void fx_close(void*c){
	struct fd_data*fx=c;
	if(!fx)return;
	if(fx->data)munmap(fx->data,fx->length);
	if(fx->auto_close&&fx->fd>=0)close(fx->fd);
	free(fx);
}

enum MHD_Result http_ret_fd_file(
	struct http_hand_info*i,
	int fd,
	const char*name,
	bool auto_close
){
	time_t mt;
	char mime[128];
	struct stat st;
	void*buffer=NULL;
	int code=MHD_HTTP_OK;
	size_t len=0,start=0,end=0;
	struct MHD_Response*r=NULL;
	if(!i||fd<0||!name)return MHD_NO;
	if(fstat(fd,&st)!=0)return MHD_NO;
	mt=st.st_mtim.tv_sec;
	if(http_check_last_modify(i,mt)==MHD_YES)
		return MHD_YES;
	len=st.st_size,end=len-1;
	mime_get_by_filename(mime,sizeof(mime),name);
	if(http_parse_range(i,&code,st.st_size,&start,&end))
		len=end+1-start;
	if(code!=MHD_HTTP_OK&&code!=MHD_HTTP_PARTIAL_CONTENT)
		len=0;
	if(len>0&&(buffer=mmap(
		NULL,st.st_size,PROT_READ,
		MAP_SHARED,fd,0
	))==MAP_FAILED){
		telog_debug("mmap file %s failed",name);
		return MHD_NO;
	}
	if(
		http_check_can_deflate(i,len,mime)&&
		(r=http_create_zlib_response(buffer,len,MHD_RESPMEM_PERSISTENT))
	){
		munmap(buffer,len);
		if(auto_close)close(fd);
	}
	if(!r){
		struct fd_data*fx;
		if(!(fx=malloc(sizeof(struct fd_data)))){
			munmap(buffer,len);
			return MHD_NO;
		}
		fx->fd=fd,fx->auto_close=auto_close;
		fx->data=buffer,fx->length=st.st_size;
		r=MHD_create_response_from_buffer_with_free_callback_cls(
			len,buffer+start,fx_close,fx
		);
	}
	http_add_file_name_header(r,name);
	http_add_range(r,code,start,end,st.st_size);
	if(code==MHD_HTTP_OK)http_add_time_header(
		r,MHD_HTTP_HEADER_LAST_MODIFIED,mt
	);
	MHD_add_response_header(
		r,MHD_HTTP_HEADER_CONTENT_TYPE,mime
	);
	enum MHD_Result x=MHD_queue_response(i->conn,code,r);
	MHD_destroy_response(r);
	return x;
}

enum MHD_Result http_ret_file(
	struct http_hand_info*i,
	const char*path
){
	int fd=open(path,O_RDONLY|O_CLOEXEC);
	if(fd<0){
		telog_warn("open %s failed",path);
		return MHD_NO;
	}
	return http_ret_fd_file(i,fd,path,true);
}

enum MHD_Result http_ret_std_file(
	struct http_hand_info*i,
	FILE*file,
	const char*name
){
	return file?http_ret_fd_file(
		i,fileno(file),name,false
	):MHD_NO;
}

enum MHD_Result http_hand_file(struct http_hand_info*i){
	if(!i||!i->hand)return MHD_NO;
	struct http_hand_file*f=&i->hand->spec.file;
	if(!f->name)return MHD_NO;
	if(f->file)return http_ret_std_file(i,f->file,f->name);
	if(f->fd>0)return http_ret_fd_file(i,f->fd,f->name,false);
	return http_ret_file(i,f->name);
}

enum MHD_Result http_ret_fd_folder(
	struct http_hand_info*i,
	int fd,
	const char**index
){
	int tgt=-1;
	struct stat st;
	const char*name=NULL;
	char url[PATH_MAX],*u=(char*)i->url;
	if(!i||fd<=0)return MHD_NO;
	if(strstr(u,"/../"))return MHD_NO;
	if(strcmp(u,"/..")==0)return MHD_NO;
	if(strcmp(u,"../")==0)return MHD_NO;
	if(strcmp(u,"..")==0)return MHD_NO;
	while(u[0]=='/')u++;
	strncpy(url,u,sizeof(url)-1);
	if(url[0]&&fstatat(
		fd,url,&st,AT_SYMLINK_NOFOLLOW
	)!=0){
		if(errno==ENOENT)tlog_verbose("%s not found",url);
		else telog_debug("stat %s failed",url);
		return MHD_NO;
	}
	if(!url[0]||S_ISDIR(st.st_mode)){
		int sub=url[0]?openat(
			fd,url,
			O_RDONLY|O_DIRECTORY|O_CLOEXEC
		):fd;
		if(sub<=0){
			if(errno==ENOENT)tlog_verbose("%s not found",url);
			else telog_debug("open %s failed",url);
			return MHD_NO;
		}
		if(strlen(i->url)>1&&url[strlen(url)-1]!='/'){
			strlcat(url,"/",sizeof(url)-1);
			if(url[0])close(sub);
			return http_ret_redirect(
				i,MHD_HTTP_MOVED_PERMANENTLY,url
			);
		}
		if(index)for(int x=0;index[x];x++){
			name=index[x];
			if((tgt=openat(
				sub,index[x],
				O_RDONLY|O_CLOEXEC
			))>=0)break;
			else if(errno!=ENOENT)telog_debug(
				"open %s/%s failed",url,index[x]
			);
		}
		if(url[0])close(sub);
	}else if(S_ISREG(st.st_mode)){
		name=url,tgt=openat(fd,url,O_RDONLY|O_CLOEXEC);
		if(tgt<0&&errno!=ENOENT)telog_debug("open %s failed",url);
	}
	if(tgt<0||!name)return MHD_NO;
	return http_ret_fd_file(i,tgt,name,true);
}

enum MHD_Result http_ret_folder(
	struct http_hand_info*i,
	const char*path,
	const char**index
){
	int fd=open(path,O_RDONLY|O_DIRECTORY|O_CLOEXEC);
	if(fd<0)return MHD_NO;
	enum MHD_Result r=http_ret_fd_folder(i,fd,index);
	close(fd);
	return r;
}

enum MHD_Result http_hand_folder(struct http_hand_info*i){
	if(!i||!i->hand)return MHD_NO;
	struct http_hand_folder*f=&i->hand->spec.folder;
	if(f->fd>0)return http_ret_fd_folder(i,f->fd,f->index);
	return http_ret_folder(i,f->path,f->index);
}

void http_logger(
	void*cls __attribute__((unused)),
	const char *fm,va_list ap
){
	char*buf=NULL;
	vasprintf(&buf,fm,ap);
	if(!buf)return;
	logger_print(LEVEL_INFO,TAG,buf);
	free(buf);
}

enum MHD_Result http_conn_handler(
	void*c,struct MHD_Connection*o,
	const char*u,const char*m,const char*v,
	const char*d,size_t*s,void**n
){
	struct http_hand_info hand={
		.cls=c,.conn=o,.url=u,.method=m,
		.version=v,.data=d,.data_size=s,.con_cls=n,
	};
	struct http_hand*hands=c;
	enum MHD_Result r=MHD_NO;
	for(size_t i=0;hands[i].enabled;i++){
		if(hands[i].url&&strcasecmp(u,hands[i].url)!=0)continue;
		if(hands[i].map){
			size_t l=strlen(hands[i].map);
			if(strncasecmp(u,hands[i].map,l)!=0)continue;
			hand.url+=l;
		}
		if(hands[i].regex){
			Reprog*prog;
			bool match=false;
			if(!(prog=regexp_comp(hands[i].regex,REG_ICASE,NULL)))
				return http_ret_code(&hand,MHD_HTTP_INTERNAL_SERVER_ERROR);
			if(regexp_exec(prog,u,NULL,0)==0)match=true;
			regexp_free(prog);
			if(!match)continue;
		}
		if(hands[i].method&&strcasecmp(m,hands[i].method)!=0)
			return http_ret_code(&hand,MHD_HTTP_BAD_REQUEST);
		hand.hand=&hands[i];
		r=hands[i].handler(&hand);
		if(r==MHD_YES)break;
	}
	return r==MHD_YES?r:http_ret_code(&hand,MHD_HTTP_NOT_FOUND);
}
#endif
