/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LIBCURL
#include<curl/curl.h>
#include"../fs_internal.h"

struct mem_data{
	bool allocate;
	void*buffer;
	size_t size;
	size_t pos;
};

struct curl_ctx{
	char*path;
	CURL*hand;
	size_t pos;
	size_t size;
	bool have_size;
};

static size_t mem_write_cb(
	void*cont,
	size_t size,
	size_t nb,
	void*data
){
	struct mem_data*mem=data;
	size_t rs=MIN(
		size*nb,
		mem->size-mem->pos
		);
	if(rs==0)return 0;
	memcpy(
		mem->buffer+
		mem->pos,
		cont,rs
	);
	mem->pos+=rs;
	return rs;
}

static size_t mem_read_cb(
	void*cont,
	size_t size,
	size_t nb,
	void*data
){
	struct mem_data*mem=data;
	size_t rs=MIN(
		size*nb,
		mem->size-
		mem->pos
	);
	if(rs==0)return 0;
	memcpy(
		cont,
		mem->buffer+
		mem->pos,
		rs
	);
	mem->pos+=rs;
	return rs;
}

static int curl_code_to_errno(CURLcode code){
	switch(code){
		case CURLE_OK:RET(0);
		case CURLE_UNSUPPORTED_PROTOCOL:RET(EPROTONOSUPPORT);
		case CURLE_NO_CONNECTION_AVAILABLE:RET(EMFILE);
		case CURLE_ABORTED_BY_CALLBACK:RET(ECANCELED);
		case CURLE_REMOTE_FILE_EXISTS:RET(EEXIST);
		case CURLE_UNRECOVERABLE_POLL:RET(EFAULT);
		case CURLE_REMOTE_DISK_FULL:RET(ENOSPC);
		case CURLE_FILESIZE_EXCEEDED:RET(EFBIG);
		case CURLE_TFTP_NOSUCHUSER:RET(EUSERS);
		case CURLE_TFTP_UNKNOWNID:RET(EBADF);
		case CURLE_RANGE_ERROR:RET(ERANGE);
		case CURLE_AGAIN:RET(EAGAIN);
		case CURLE_COULDNT_RESOLVE_PROXY:
		case CURLE_COULDNT_RESOLVE_HOST:
		case CURLE_FTP_CANT_GET_HOST:RET(EDESTADDRREQ);
		case CURLE_AUTH_ERROR:
		case CURLE_LOGIN_DENIED:
		case CURLE_SSL_ISSUER_ERROR:
		case CURLE_SSL_INVALIDCERTSTATUS:
		case CURLE_SSL_PINNEDPUBKEYNOTMATCH:
		case CURLE_REMOTE_ACCESS_DENIED:RET(EACCES);
		case CURLE_FTP_ACCEPT_TIMEOUT:
		case CURLE_OPERATION_TIMEDOUT:RET(ETIMEDOUT);
		case CURLE_GOT_NOTHING:
		case CURLE_QUOTE_ERROR:
		case CURLE_URL_MALFORMAT:
		case CURLE_UNKNOWN_OPTION:
		case CURLE_SETOPT_OPTION_SYNTAX:
		case CURLE_BAD_FUNCTION_ARGUMENT:
		case CURLE_SSL_CRL_BADFILE:
		case CURLE_SSL_CACERT_BADFILE:
		case CURLE_FTP_BAD_FILE_LIST:
		case CURLE_TFTP_ILLEGAL:RET(EINVAL);
		case CURLE_FAILED_INIT:
		case CURLE_OUT_OF_MEMORY:RET(ENOMEM);
		case CURLE_FTP_PORT_FAILED:
		case CURLE_COULDNT_CONNECT:
		case CURLE_LDAP_CANNOT_BIND:
		case CURLE_SSL_CONNECT_ERROR:
		case CURLE_SSL_SHUTDOWN_FAILED:RET(ECONNABORTED);
		case CURLE_NOT_BUILT_IN:
		case CURLE_BAD_CONTENT_ENCODING:RET(ENOTSUP);
		case CURLE_SSL_CIPHER:
		case CURLE_FUNCTION_NOT_FOUND:
		case CURLE_SSL_ENGINE_NOTFOUND:
		case CURLE_SSL_ENGINE_SETFAILED:
		case CURLE_SSL_ENGINE_INITFAILED:RET(ENOSYS);
		case CURLE_TFTP_NOTFOUND:
		case CURLE_LDAP_SEARCH_FAILED:
		case CURLE_REMOTE_FILE_NOT_FOUND:RET(ENOENT);
		case CURLE_RECURSIVE_API_CALL:
		case CURLE_TOO_MANY_REDIRECTS:RET(ELOOP);
		case CURLE_SSL_CERTPROBLEM:
		case CURLE_UPLOAD_FAILED:
		case CURLE_WRITE_ERROR:
		case CURLE_RECV_ERROR:
		case CURLE_READ_ERROR:
		case CURLE_SEND_ERROR:
		case CURLE_SEND_FAIL_REWIND:
		case CURLE_FILE_COULDNT_READ_FILE:
		case CURLE_FTP_WEIRD_PASS_REPLY:
		case CURLE_FTP_WEIRD_PASV_REPLY:
		case CURLE_FTP_WEIRD_227_FORMAT:
		case CURLE_BAD_DOWNLOAD_RESUME:
		case CURLE_WEIRD_SERVER_REPLY:
		case CURLE_HTTP2_STREAM:RET(EIO);
		case CURLE_PEER_FAILED_VERIFICATION:
		case CURLE_SSL_CLIENTCERT:
		case CURLE_TFTP_PERM:RET(EPERM);
		case CURLE_CONV_FAILED:
		case CURLE_PARTIAL_FILE:
		case CURLE_CHUNK_FAILED:
		case CURLE_USE_SSL_FAILED:
		case CURLE_INTERFACE_FAILED:
		case CURLE_FTP_PRET_FAILED:
		case CURLE_FTP_ACCEPT_FAILED:
		case CURLE_FTP_COULDNT_SET_TYPE:
		case CURLE_FTP_COULDNT_RETR_FILE:
		case CURLE_FTP_COULDNT_USE_REST:
		case CURLE_RTSP_SESSION_ERROR:
		case CURLE_RTSP_CSEQ_ERROR:
		case CURLE_QUIC_CONNECT_ERROR:
		case CURLE_HTTP_RETURNED_ERROR:
		case CURLE_HTTP_POST_ERROR:
		case CURLE_HTTP2:
		case CURLE_HTTP3:
		case CURLE_PROXY:
		case CURLE_SSH:RET(EPROTO);
		default:RET(EBADMSG);
	}
}

static void update_info(struct curl_ctx*ctx){
	CURLcode c;
	off_t len=0;
	c=curl_easy_getinfo(
		ctx->hand,
		CURLINFO_CONTENT_LENGTH_DOWNLOAD_T,
		&len
	);
	if(c==CURLE_OK){
		ctx->size=len;
		ctx->have_size=true;
	}
}

static int fsdrv_open(
	const fsdrv*drv,
	fsh*nf,
	url*uri,
	fs_file_flag flags
){
	int e=0;
	CURLcode c;
	struct curl_ctx*ctx;
	if(fs_has_flag(flags,FILE_FLAG_NON_BLOCK))RET(ENOTSUP);
	if(!drv||!nf||!uri||!(ctx=nf->data))RET(EINVAL);
	if(!(ctx->hand=curl_easy_init()))DONE(ENOMEM);
	if(!(ctx->path=url_generate_alloc(uri)))DONE(ENOMEM);
	curl_easy_setopt(ctx->hand,CURLOPT_URL,ctx->path);
	curl_easy_setopt(ctx->hand,CURLOPT_NOBODY,1L);
	curl_easy_setopt(ctx->hand,CURLOPT_FOLLOWLOCATION,1L);
	if((c=curl_easy_perform(ctx->hand))!=CURLE_OK){
		tlog_warn("request %s for info failed",ctx->path);
		DONE(curl_code_to_errno(c));
	}
	update_info(ctx);
	RET(0);
	done:e=errno;
	if(ctx->path)free(ctx->path);
	if(ctx->hand)curl_easy_cleanup(ctx->hand);
	XRET(e,EIO);
}

static void set_range(struct curl_ctx*ctx,size_t size){
	char range[64];
	snprintf(
		range,sizeof(range),"%zu-%zu",
		ctx->pos,ctx->pos+size-1
	);
	curl_easy_setopt(ctx->hand,CURLOPT_RANGE,range);
}

static int proc_return(
	struct curl_ctx*ctx,
	struct mem_data*md,
	const char*req
){
	long code=0;
	curl_easy_getinfo(
		ctx->hand,
		CURLINFO_RESPONSE_CODE,
		&code
	);
	if(code==200){
		ctx->pos=md->pos;
		update_info(ctx);
	}else if(code==206)ctx->pos+=md->pos;
	else if(code>=400){
		tlog_warn(
			"%s request %s failed with code %ld",
			req,ctx->path,code
		);
		switch(code){
			case 400:RET(EINVAL);
			case 401:RET(EACCES);
			case 403:RET(EPERM);
			case 404:RET(ENOENT);
			case 416:RET(0);
			default:RET(EPROTO);
		}
	}
	RET(0);
}

static int fsdrv_read(
	const fsdrv*drv,
	fsh*f,
	void*buffer,
	size_t btr,
	size_t*br
){
	CURLcode c;
	struct curl_ctx*ctx;
	struct mem_data md;
	if(br)*br=0;
	if(!drv||!f||!(ctx=f->data))RET(EINVAL);
	if(!buffer||f->driver!=drv)RET(EINVAL);
	if(!fs_has_flag(f->flags,FILE_FLAG_READ))RET(EPERM);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	md.buffer=buffer,md.size=btr;
	md.pos=0,md.allocate=false;
	curl_easy_setopt(ctx->hand,CURLOPT_NOBODY,0L);
	curl_easy_setopt(ctx->hand,CURLOPT_READFUNCTION,NULL);
	curl_easy_setopt(ctx->hand,CURLOPT_WRITEFUNCTION,mem_write_cb);
	curl_easy_setopt(ctx->hand,CURLOPT_READDATA,NULL);
	curl_easy_setopt(ctx->hand,CURLOPT_WRITEDATA,&md);
	set_range(ctx,btr);
	c=curl_easy_perform(ctx->hand);
	if(br)*br=md.pos;
	if(c!=CURLE_OK){
		tlog_warn(
			"read request %s failed: %s",
			ctx->path,curl_easy_strerror(c)
		);
		RET(curl_code_to_errno(c));
	}
	return proc_return(ctx,&md,"read");
}

static int fsdrv_write(
	const fsdrv*drv,
	fsh*f,
	void*buffer,
	size_t btw,
	size_t*bw
){
	CURLcode c;
	struct curl_ctx*ctx;
	struct mem_data md;
	if(!f||!(ctx=f->data)||!drv)RET(EINVAL);
	if(!buffer||f->driver!=drv)RET(EINVAL);
	md.buffer=buffer,md.size=btw,md.pos=0,md.allocate=false;
	if(!fs_has_flag(f->flags,FILE_FLAG_WRITE))RET(EPERM);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	curl_easy_setopt(ctx->hand,CURLOPT_NOBODY,0L);
	curl_easy_setopt(ctx->hand,CURLOPT_UPLOAD,1L);
	curl_easy_setopt(ctx->hand,CURLOPT_READFUNCTION,mem_read_cb);
	curl_easy_setopt(ctx->hand,CURLOPT_WRITEFUNCTION,NULL);
	curl_easy_setopt(ctx->hand,CURLOPT_READDATA,&md);
	curl_easy_setopt(ctx->hand,CURLOPT_WRITEDATA,NULL);
	set_range(ctx,btw);
	c=curl_easy_perform(ctx->hand);
	if(bw)*bw=md.pos;
	if(c!=CURLE_OK){
		tlog_warn(
			"write request %s failed: %s",
			ctx->path,curl_easy_strerror(c)
		);
		RET(curl_code_to_errno(c));
	}
	return proc_return(ctx,&md,"write");
}

static int fsdrv_get_size(const fsdrv*drv,fsh*f,size_t*out){
	struct curl_ctx*ctx;
	if(!f||!(ctx=f->data))RET(EINVAL);
	if(!drv||!out||f->driver!=drv)RET(EINVAL);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	if(!ctx->have_size)RET(ENOTSUP);
	*out=ctx->size;
	RET(0);
}

static int fsdrv_seek(const fsdrv*drv,fsh*f,size_t pos,int whence){
	struct curl_ctx*ctx;
	if(!f||!(ctx=f->data))RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	switch(whence){
		case SEEK_SET:ctx->pos=pos;break;
		case SEEK_CUR:ctx->pos+=pos;break;
		case SEEK_END:
			if(ctx->have_size&&ctx->pos>ctx->size)
				ctx->pos=pos+ctx->size;
		break;
	}
	return errno;
}

static int fsdrv_tell(const fsdrv*drv,fsh*f,size_t*pos){
	struct curl_ctx*ctx;
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	if(!(ctx=f->data)||!pos)RET(EINVAL);
	*pos=ctx->pos;
	return errno;
}

static void fsdrv_close(const fsdrv*drv,fsh*f){
	struct curl_ctx*ctx;
	if(!f||!(ctx=f->data))return;
	if(!drv||f->driver!=drv)return;
	if(ctx->hand)curl_easy_cleanup(ctx->hand);
	if(ctx->path)free(ctx->path);
}

static fsdrv fsdrv_curl={
	.magic=FS_DRIVER_MAGIC,
	.base=&fsdrv_template,
	.hand_data_size=sizeof(struct curl_ctx),
	.features=
		FS_FEATURE_READABLE|
		FS_FEATURE_WRITABLE|
		FS_FEATURE_SEEKABLE|
		FS_FEATURE_HAVE_SIZE|
		FS_FEATURE_HAVE_PATH,
	.open=fsdrv_open,
	.seek=fsdrv_seek,
	.tell=fsdrv_tell,
	.close=fsdrv_close,
	.read=fsdrv_read,
	.write=fsdrv_write,
	.get_size=fsdrv_get_size,
};

void fsdrv_register_curl(bool deinit){
	errno=0;
	fsdrv*drv=NULL;
	curl_version_info_data*data=NULL;
	if(deinit)curl_global_cleanup();
	else{
		curl_global_init(CURL_GLOBAL_ALL);
		if(!(data=curl_version_info(CURLVERSION_NOW)))return;
		for(size_t i=0;data->protocols[i];i++){
			if(!(drv=malloc(sizeof(fsdrv))))continue;
			memcpy(drv,&fsdrv_curl,sizeof(fsdrv));
			strncpy(
				drv->protocol,
				data->protocols[i],
				sizeof(drv->protocol)-1
			);
			if(fsdrv_register(drv)==0)continue;
			free(drv);
		}
	}
}
#endif
