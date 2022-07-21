/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_MICROHTTPD
#ifdef ENABLE_WEBSOCKET
#include<stdio.h>
#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<strings.h>
#include<microhttpd.h>
#include<microhttpd_ws.h>
#include"logger.h"
#include"system.h"
#include"assets.h"
#include"array.h"
#include"lock.h"
#include"http.h"
#include"str.h"
#define TAG "websocket"

struct ws_pkt_cmd;
struct packet_data;
typedef int(*cmd_hand)(
	struct ws_pkt_cmd*,
	struct packet_data*,
	struct http_hand_websocket_data*,
	char**,size_t*
);
struct packet_data{
	char tag[64];
	char*data;
	size_t size,pos;
};
struct ws_pkt_cmd{
	const char cmd[64];
	const char ret[64];
	cmd_hand hand;
	int code;
};

int ws_write(
	struct http_hand_websocket_data*w,
	const char*data,
	size_t len
){
	int r;
	char*dd=NULL;
	size_t dl=0;
	if(!data||!w||!w->ws||w->fd<0)return -1;
	MUTEX_LOCK(w->lock);
	if((r=MHD_websocket_encode_binary(
		w->ws,data,len,0,&dd,&dl
	))==0){
		r=full_write(w->fd,dd,dl);
		MHD_websocket_free(w->ws,dd);
	}
	MUTEX_UNLOCK(w->lock);
	return r;
}

int ws_print(
	struct http_hand_websocket_data*w,
	const char*data
){
	return data?ws_write(w,data,strlen(data)):-1;
}

int ws_printf(
	struct http_hand_websocket_data*w,
	const char*fmt,...
){
	char*d=NULL;
	if(!fmt)return -1;
	va_list va;
	va_start(va,fmt);
	int r=vasprintf(&d,fmt,va);
	va_end(va);
	if(!d)return -1;
	if(r>0)r=ws_write(w,d,r);
	free(d);
	return r;
}

int ws_send_cmd(
	struct http_hand_websocket_data*w,
	const char*cmd
){
	tlog_verbose("send cmd %s",cmd);
	return ws_printf(w,"!#%-5s\n",cmd);
}

int ws_send_cmd_r(
	int r,
	struct http_hand_websocket_data*w,
	const char*cmd
){
	ws_send_cmd(w,cmd);
	return r;
}

static int parse_packet_data_head(
	struct ws_pkt_cmd*cmd __attribute__((unused)),
	struct packet_data*d,
	struct http_hand_websocket_data*w,
	char**dd,size_t*dl
){
	char*p;
	ssize_t ss=0;
	if(!(p=strchr(*dd,':')))
		return ws_send_cmd_r(1,w,"INVAL");
	*p++=0;
	strncpy(d->tag,*dd,sizeof(d->tag)-1);
	*dl-=p-(*dd),*dd=p;
	if(!(p=strchr(*dd,';')))
		return ws_send_cmd_r(1,w,"INVAL");
	*p++=0;
	if((ss=parse_long(*dd,0))<=0)
		return ws_send_cmd_r(1,w,"INVAL");
	*dl-=p-(*dd),*dd=p;
	if(!(d->data=malloc(ss+1)))
		return ws_send_cmd_r(1,w,"ERROR");
	d->size=(size_t)ss;
	memset(d->data,0,ss+1);
	tlog_verbose("receive %zu bytes data with tag %s",ss,d->tag);
	return 0;
}

static int pkt_cmd(
	struct ws_pkt_cmd*cmd,
	struct packet_data*d __attribute__((unused)),
	struct http_hand_websocket_data*w,
	char**dd __attribute__((unused)),
	size_t*dl __attribute__((unused))
){
	return !cmd->ret[0]?cmd->code:
		ws_send_cmd_r(cmd->code,w,cmd->cmd);
}

static struct ws_pkt_cmd pkt_cmds[]={
	{.cmd="OKAY",.code=0},
	{.cmd="ERROR",.code=0},
	{.cmd="INVAL",.code=0},
	{.cmd="UNSUP",.code=0},
	{.cmd="CLOSE",.ret="OKAY",.code=2},
	{.cmd="EXIT",.ret="OKAY",.code=2},
	{.cmd="PING",.ret="PONG",.code=0},
	{.cmd="DATA@",.hand=parse_packet_data_head},
};

static int parse_packet_head(
	struct packet_data*d,
	struct http_hand_websocket_data*w,
	char**dd,size_t*dl
){
	if(d->data)free(d->data);
	memset(d,0,sizeof(struct packet_data));
	if(*dl<3||(*dd)[0]!='!'||(*dd)[1]!='#')return 1;
	*dd+=2,*dl-=2;
	for(size_t i=0;i<ARRLEN(pkt_cmds);i++){
		cmd_hand hand=pkt_cmds[i].hand;
		const char*cmd=pkt_cmds[i].cmd;
		if(!cmd[0])break;
		size_t len=strlen(cmd);
		if(len>*dl)continue;
		if(strncasecmp(*dd,cmd,len)!=0)continue;
		*dd+=len,*dl-=len;
		if(!hand)hand=pkt_cmd;
		tlog_verbose("receive cmd %s",cmd);
		return hand(&pkt_cmds[i],d,w,dd,dl);
	}
	if(w->hand->cmds)for(size_t i=0;w->hand->cmds[i];i++){
		const char*cmd=w->hand->cmds[i]->cmd;
		ws_cmd_hand hand=w->hand->cmds[i]->hand;
		if(!cmd[0]||!hand)break;
		size_t len=strlen(cmd);
		if(len>*dl)continue;
		if(strncasecmp(*dd,cmd,len)!=0)continue;
		*dd+=len,*dl-=len;
		tlog_verbose("receive cmd %s",cmd);
		return hand(w->hand->cmds[i],w,dd,dl);
	}
	return ws_send_cmd_r(1,w,"UNSUP");
}

int ws_send_payload(
	struct http_hand_websocket_data*w,
	const char*tag,
	const void*payload,
	size_t len
){
	if(!payload)return -1;
	tlog_verbose("send %zu bytes data with tag %s",len,tag);
	int r=ws_printf(w,"!#DATA@%s:%zu;",tag,len);
	if(r>0)r+=ws_write(w,payload,len);
	return r;
}

int ws_print_payload(
	struct http_hand_websocket_data*w,
	const char*tag,
	const char*payload
){
	return payload?ws_send_payload(
		w,tag,payload,strlen(payload)
	):-1;
}

int ws_send_json_payload(
	struct http_hand_websocket_data*w,
	const char*tag,
	json_object*jo
){
	return jo?ws_print_payload(w,tag,
		json_object_to_json_string(jo)):-1;
}

static int call_data_hand(
	struct http_hand_websocket_data*w,
	struct ws_data_hand*hand,
	struct packet_data*d
){
	int ret=0;
	if(hand->recv_data)ret=hand->recv_data(
		w,hand,d->data,d->size
	);
	if(hand->recv_json){
		json_object*jo=json_tokener_parse(d->data);
		if(jo){
			ret=hand->recv_json(w,hand,jo);
			json_object_put(jo);
		}
	}
	return ret;
}

static int proc_data(
	struct http_hand_websocket_data*w,
	struct packet_data*d,
	char**dd,size_t*dl
){
	int ret=0;
	if(!dd||!dl)return -1;
	while(*dl>0){
		if(!*dd)break;
		if(d->size==0||d->data==NULL)
			ret=parse_packet_head(d,w,dd,dl);
		if(ret!=0)break;
		if(d->data&&d->size>0&&d->size>d->pos){
			size_t c=MIN(d->size-d->pos,*dl);
			memcpy(d->data+d->pos,*dd,c);
			*dd+=c,*dl-=c,d->pos+=c;
		}
		if(d->data&&d->size<=d->pos){
			if(w->hand->hands)for(size_t i=0;w->hand->hands[i];i++){
				if(strcmp(w->hand->hands[i]->tag,d->tag)!=0)continue;
				ret=call_data_hand(w,w->hand->hands[i],d);
			}
			free(d->data);
			memset(d,0,sizeof(struct packet_data));
			ws_send_cmd(w,"OKAY");
		}
	}
	return ret;
}

static void send_close(
	struct http_hand_websocket_data*w
){
	size_t sl=0;
	char *sd=NULL;
	MUTEX_LOCK(w->lock);
	if(MHD_websocket_encode_close(
		w->ws,0,NULL,0,&sd,&sl
	)==0){
		full_write(w->fd,sd,sl);
		MHD_websocket_free(w->ws,sd);
	}
	MUTEX_UNLOCK(w->lock);
}

static void send_pong(
	struct http_hand_websocket_data*w,
	char**dd,size_t*dl
){
	size_t sl=0;
	char *sd=NULL;
	MUTEX_LOCK(w->lock);
	if(MHD_websocket_encode_pong(
		w->ws,*dd,*dl,&sd,&sl
	)==0){
		full_write(w->fd,sd,sl);
		MHD_websocket_free(w->ws,sd);
	}
	MUTEX_UNLOCK(w->lock);
}

static int proc_ws_data(
	struct http_hand_websocket_data*w,
	struct packet_data*d,
	enum MHD_WEBSOCKET_STATUS st,
	char**dd,size_t*dl
){
	int ret=0;
	switch(st){
		case MHD_WEBSOCKET_STATUS_BINARY_FRAME:
		case MHD_WEBSOCKET_STATUS_TEXT_FRAME:ret=proc_data(w,d,dd,dl);break;
		case MHD_WEBSOCKET_STATUS_PING_FRAME:send_pong(w,dd,dl);break;
		case MHD_WEBSOCKET_STATUS_CLOSE_FRAME:send_close(w);break;
		default:tlog_warn("unsupported packet type %d",st);break;
	}
	return ret;
}

int ws_close(struct http_hand_websocket_data*w){
	if(!w)return -1;
	if(!w->connected)return 0;
	ws_send_cmd(w,"CLOSE");
	send_close(w);
	w->connected=false;
	return 0;
}

bool ws_is_valid(struct http_hand_websocket_data*w){
	return w&&w->ws&&w->connected&&
		MHD_websocket_stream_is_valid(w->ws)==
		MHD_WEBSOCKET_VALIDITY_VALID;
}

static void upgrade_hand_ws(
	void*cls,
	struct MHD_Connection*con __attribute__((unused)),
	void*con_cls __attribute__((unused)),
	const char*extra_in __attribute__((unused)),
	size_t extra_in_size __attribute__((unused)),
	MHD_socket fd,
	struct MHD_UpgradeResponseHandle*urh
){
	fd_set fds;
	ssize_t got;
	size_t dl,no;
	char buf[256],*bd,*dd;
	struct timeval timeout;
	struct packet_data data;
	struct http_hand_websocket_data wsd;
	memset(&wsd,0,sizeof(wsd));
	memset(&data,0,sizeof(data));
	wsd.info=cls,wsd.fd=fd;
	wsd.hand=&wsd.info->spec.websocket;
	int r=fcntl(fd,F_GETFL);
	if(r!=-1&&(r&~O_NONBLOCK)!=r)
		fcntl(fd,F_SETFL,r&~O_NONBLOCK);
	if((r=MHD_websocket_stream_init(&wsd.ws,0,0))!=0){
		MHD_upgrade_action(urh,MHD_UPGRADE_ACTION_CLOSE);
		return;
	}
	MUTEX_INIT(wsd.lock);
	wsd.connected=true;
	if(wsd.hand->establish)
		wsd.connected=wsd.hand->establish(&wsd)==0;
	bd=dd=NULL,dl=no=0;
	while(ws_is_valid(&wsd)){
		FD_ZERO(&fds);
		FD_SET(fd,&fds);
		timeout.tv_sec=0,timeout.tv_usec=500000;
		r=select(FD_SETSIZE,&fds,NULL,NULL,&timeout);
		if(r<0)break;
		if(!FD_ISSET(fd,&fds))continue;
		MUTEX_LOCK(wsd.lock);
		got=recv(fd,buf,sizeof(buf),0);
		MUTEX_UNLOCK(wsd.lock);
		if(got<=0)break;
		size_t off=0;
		while(off<(size_t)got){
			bd=dd=NULL,dl=no=0;
			if((r=MHD_websocket_decode(
				wsd.ws,buf+off,((size_t)got)-off,
				&no,&bd,&dl
			))<0){
				if(dd)MHD_websocket_free(wsd.ws,dd);
				break;
			}
			dd=bd,off+=no;
			if(proc_ws_data(
				&wsd,&data,r,&dd,&dl
			)==2)goto end;
			if(bd)MHD_websocket_free(wsd.ws,bd);
			bd=dd=NULL;
		}
		if(bd)MHD_websocket_free(wsd.ws,bd);
		bd=dd=NULL;
	}
	end:
	if(bd)MHD_websocket_free(wsd.ws,bd);
	wsd.connected=false;
	if(wsd.hand->disconnect)wsd.hand->disconnect(&wsd);
	MUTEX_DESTROY(wsd.lock);
	MHD_websocket_stream_free(wsd.ws);
	MHD_upgrade_action(urh,MHD_UPGRADE_ACTION_CLOSE);
}

enum MHD_Result http_hand_websocket(struct http_hand_info*i){
	enum MHD_Result ret;
	char ws_accept[256];
	struct MHD_Response*r;
	if(MHD_websocket_check_http_version(i->version)!=0)goto fail;
	if(MHD_websocket_check_connection_header(MHD_lookup_connection_value(
		i->conn,MHD_HEADER_KIND,MHD_HTTP_HEADER_CONNECTION
	))!=0)goto fail;
	if(MHD_websocket_check_upgrade_header(MHD_lookup_connection_value(
		i->conn,MHD_HEADER_KIND,MHD_HTTP_HEADER_UPGRADE
	))!=0)goto fail;
	if(MHD_websocket_check_version_header(MHD_lookup_connection_value(
		i->conn,MHD_HEADER_KIND,MHD_HTTP_HEADER_SEC_WEBSOCKET_VERSION
	))!=0)goto fail;
	if(MHD_websocket_create_accept_header(MHD_lookup_connection_value(
		i->conn,MHD_HEADER_KIND,MHD_HTTP_HEADER_SEC_WEBSOCKET_KEY
	),ws_accept)!=0)goto fail;
	r=MHD_create_response_for_upgrade(upgrade_hand_ws,i->hand);
	MHD_add_response_header(r,MHD_HTTP_HEADER_UPGRADE,"websocket");
	MHD_add_response_header(r,MHD_HTTP_HEADER_SEC_WEBSOCKET_ACCEPT,ws_accept);
	ret=MHD_queue_response(i->conn,MHD_HTTP_SWITCHING_PROTOCOLS,r);
	MHD_destroy_response(r);
	return ret;
	fail:return http_ret_code(i,MHD_HTTP_BAD_REQUEST);
}

#endif
#endif
