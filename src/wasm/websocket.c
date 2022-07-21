/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<stdio.h>
#include<stdarg.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>
#include"websocket.h"

struct ws_pkt_cmd;
struct packet_data;
typedef int(*cmd_hand)(
	struct ws_pkt_cmd*,
	struct packet_data*,
	struct http_hand_websocket*,
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

long parse_long(char*str,long def){
	if(!str)return def;
	errno=0;
	char*end;
	long val=strtol(str,&end,0);
	return errno!=0||end==str?def:val;
}

int ws_write(
	struct http_hand_websocket*h,
	const char*data,
	size_t len
){
	if(!data||!h||h->ws<=0)return -1;
	if(h->counter)(*h->counter)+=len;
	emscripten_websocket_send_binary(
		h->ws,(void*)data,len
	);
	return (int)len;
}

int ws_print(
	struct http_hand_websocket*h,
	const char*data
){
	return data?ws_write(h,data,strlen(data)):-1;
}

int ws_printf(
	struct http_hand_websocket*h,
	const char*fmt,...
){
	char*d=NULL;
	if(!fmt)return -1;
	va_list va;
	va_start(va,fmt);
	int r=vasprintf(&d,fmt,va);
	va_end(va);
	if(!d)return -1;
	if(r>0)r=ws_write(h,d,r);
	free(d);
	return r;
}

int ws_send_cmd(
	struct http_hand_websocket*h,
	const char*cmd
){
	return ws_printf(h,"!#%-5s\n",cmd);
}

int ws_send_cmd_r(
	int r,
	struct http_hand_websocket*h,
	const char*cmd
){
	ws_send_cmd(h,cmd);
	return r;
}

static int parse_packet_data_head(
	struct ws_pkt_cmd*cmd __attribute__((unused)),
	struct packet_data*d,
	struct http_hand_websocket*h,
	char**dd,size_t*dl
){
	char*p;
	ssize_t ss;
	if(!(p=strchr(*dd,':')))
		return ws_send_cmd_r(1,h,"INVAL");
	*p++=0;
	strncpy(d->tag,*dd,sizeof(d->tag)-1);
	*dl-=p-(*dd),*dd=p;
	if(!(p=strchr(*dd,';')))
		return ws_send_cmd_r(1,h,"INVAL");
	*p++=0;
	if((ss=parse_long(*dd,0))<=0)
		return ws_send_cmd_r(1,h,"INVAL");
	*dl-=p-(*dd),*dd=p;
	if(!(d->data=malloc(ss+1)))
		return ws_send_cmd_r(1,h,"ERROR");
	d->size=(size_t)ss;
	memset(d->data,0,ss+1);
	return 0;
}

static int pkt_cmd(
	struct ws_pkt_cmd*cmd,
	struct packet_data*d __attribute__((unused)),
	struct http_hand_websocket*h,
	char**dd __attribute__((unused)),
	size_t*dl __attribute__((unused))
){
	return !cmd->ret[0]?cmd->code:
	       ws_send_cmd_r(cmd->code,h,cmd->cmd);
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
	struct http_hand_websocket*h,
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
		return hand(&pkt_cmds[i],d,h,dd,dl);
	}
	if(h->cmds)for(size_t i=0;h->cmds[i];i++){
		const char*cmd=h->cmds[i]->cmd;
		ws_cmd_hand hand=h->cmds[i]->hand;
		if(!cmd[0]||!hand)break;
		size_t len=strlen(cmd);
		if(len>*dl)continue;
		if(strncasecmp(*dd,cmd,len)!=0)continue;
		*dd+=len,*dl-=len;
		return hand(h->cmds[i],h,dd,dl);
	}
	return ws_send_cmd_r(1,h,"UNSUP");
}

int ws_send_payload(
	struct http_hand_websocket*h,
	const char*tag,
	const void*payload,
	size_t len
){
	if(!payload)return -1;
	int r=ws_printf(h,"!#DATA@%s:%zu;",tag,len);
	if(r>0)r+=ws_write(h,payload,len);
	return r;
}

int ws_print_payload(
	struct http_hand_websocket*h,
	const char*tag,
	const char*payload
){
	return payload?ws_send_payload(
		h,tag,payload,strlen(payload)
	):-1;
}

int ws_send_json_payload(
	struct http_hand_websocket*h,
	const char*tag,
	json_object*jo
){
	return jo?ws_print_payload(h,tag,
		json_object_to_json_string(jo)):-1;
}

static int call_data_hand(
	struct http_hand_websocket*h,
	struct ws_data_hand*hand,
	struct packet_data*d
){
	int ret=0;
	if(hand->recv_data)ret=hand->recv_data(
		h,hand,d->data,d->size
	);
	if(hand->recv_json){
		json_object*jo=json_tokener_parse(d->data);
		if(jo){
			ret=hand->recv_json(h,hand,jo);
			json_object_put(jo);
		}
	}
	return ret;
}

static int proc_data(
	struct http_hand_websocket*h,
	struct packet_data*d,
	char**dd,size_t*dl
){
	int ret=0;
	if(!dd||!dl)return -1;
	while(*dl>0){
		if(!*dd)break;
		if(d->size==0||d->data==NULL)
			ret=parse_packet_head(d,h,dd,dl);
		if(ret!=0)break;
		if(d->data&&d->size>0&&d->size>d->pos){
			size_t c=MIN(d->size-d->pos,*dl);
			memcpy(d->data+d->pos,*dd,c);
			*dd+=c,*dl-=c,d->pos+=c;
		}
		if(d->data&&d->size<=d->pos){
			if(h->hands)for(size_t i=0;h->hands[i];i++){
				if(strcmp(h->hands[i]->tag,d->tag)!=0)continue;
				ret=call_data_hand(h,h->hands[i],d);
			}
			free(d->data);
			memset(d,0,sizeof(struct packet_data));
			ws_send_cmd(h,"OKAY");
		}
	}
	return ret;
}

EM_BOOL ws_on_message(
	int type __attribute__((unused)),
	const EmscriptenWebSocketMessageEvent*event,
	void*data
){
	struct http_hand_websocket*h=data;
	static struct packet_data d={
		.data=NULL,.tag="",
		.size=0,.pos=0
	};
	char*dd=(char*)event->data;
	size_t dl=event->numBytes;
	if(!h||!dd||dl<=0||event->socket!=h->ws)return EM_FALSE;
	if(h->counter)(*h->counter)+=event->numBytes;
	proc_data(h,&d,&dd,&dl);
	return EM_TRUE;
}