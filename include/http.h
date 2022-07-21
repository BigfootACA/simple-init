/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _HTTP_H
#define _HTTP_H
#include"lock.h"
#include"list.h"
#include"keyval.h"
#include"assets.h"
#include<json.h>
#include<stdio.h>
#include<microhttpd.h>
#include<microhttpd_ws.h>
struct http_hand_info{
	void*cls;
	struct MHD_Connection*conn;
	const char*url;
	const char*method;
	const char*version;
	const char*data;
	size_t*data_size;
	void**con_cls;
	struct http_hand*hand;
};
struct http_hand_file{
	int fd;
	FILE*file;
	const char*name;
	const char*mime;
};
struct http_hand_folder{
	int fd;
	const char*path;
	const char**index;
};
struct http_hand_assets{
	entry_dir*dir;
	const char*path;
	const char**index;
};
struct http_hand_assets_file{
	struct{
		entry_file*file;
	}by_file;
	struct{
		entry_dir*dir;
		const char*path;
	}by_path;
};
struct http_hand_websocket_data{
	bool connected;
	mutex_t lock;
	struct http_hand_websocket*hand;
	struct http_hand*info;
	struct MHD_WebSocketStream*ws;
	int fd;
};
struct ws_cmd_proc;
struct ws_data_hand;
typedef int(*ws_cmd_hand)(
	struct ws_cmd_proc*,
	struct http_hand_websocket_data*,
	char**,size_t*
);
struct ws_cmd_proc{
	const char cmd[64];
	ws_cmd_hand hand;
};
typedef int(*ws_json_handler)(
	struct http_hand_websocket_data*w,
	struct ws_data_hand*hand,
	const json_object*jo
);
typedef int(*ws_data_handler)(
	struct http_hand_websocket_data*w,
	struct ws_data_hand*hand,
	const char*data,
	size_t len
);
typedef int(*ws_status_handler)(
	struct http_hand_websocket_data*
);

struct ws_data_hand{
	char tag[64];
	ws_json_handler recv_json;
	ws_data_handler recv_data;
};

#define WS_CMDS(cmds...)((struct ws_cmd_proc*[]){cmds NULL})
#define WS_HANDS(hands...)((struct ws_data_hand*[]){hands NULL})
#define WS_CMD_PROC(_name,_hand)(&(struct ws_cmd_proc){.cmd=_name,.hand=_hand}),
#define WS_HAND_PROC(_name,_json,_data)(&(struct ws_data_hand){.tag=_name,.recv_json=_json,.recv_data=_data}),
#define WS_CMD_ONE(_name,_hand)WS_CMDS(WS_CMD_PROC(_name,_hand))
#define WS_HAND_ONE(_name,_hand)WS_HANDS(WS_HAND_PROC(_name,_hand))
struct http_hand_websocket{
	ws_status_handler establish;
	ws_status_handler disconnect;
 	struct ws_data_hand**hands;
	struct ws_cmd_proc**cmds;
};
typedef enum MHD_Result(*http_handler)(struct http_hand_info*);
struct http_hand{
	bool enabled;
	const char*url;
	const char*map;
	const char*regex;
	union{
		void*data;
		#ifdef ASSETS_H
		struct http_hand_assets assets;
		struct http_hand_assets_file assets_file;
		#endif
		struct http_hand_websocket websocket;
		struct http_hand_folder folder;
		struct http_hand_file file;
		int code;
	}spec;
	const char*method;
	http_handler handler;
};
extern void http_logger(void*,const char*,va_list);
extern void http_add_time_header(struct MHD_Response*r,const char*key,time_t t);
extern void http_add_file_name_header(struct MHD_Response*r,const char*name);
extern void http_add_range(struct MHD_Response*r,int code,size_t start,size_t end,size_t len);
extern bool http_parse_range(struct http_hand_info*i,int*code,size_t len,size_t*start,size_t*end);
extern bool http_check_can_deflate(struct http_hand_info*i,size_t len,const char*mime);
extern bool http_has_deflate(struct http_hand_info*i);
extern list*http_get_encodings(struct http_hand_info*i);
extern struct MHD_Response*http_create_zlib_response(void*buf,size_t len,enum MHD_ResponseMemoryMode m);
extern enum MHD_Result http_check_last_modify(struct http_hand_info*i,time_t time);
extern enum MHD_Result http_ret_code(struct http_hand_info*i,int code);
extern enum MHD_Result http_ret_code_headers(struct http_hand_info*i,int code,keyval**kvs);
extern enum MHD_Result http_ret_redirect(struct http_hand_info*i,int code,const char*path);
extern enum MHD_Result http_ret_assets_file(struct http_hand_info*i,entry_file*file);
extern enum MHD_Result http_ret_assets_folder(struct http_hand_info*i,entry_dir*root,const char**index);
extern enum MHD_Result http_ret_fd_file(struct http_hand_info*i,int fd,const char*name,bool auto_close);
extern enum MHD_Result http_ret_fd_folder(struct http_hand_info*i,int fd,const char**index);
extern enum MHD_Result http_ret_folder(struct http_hand_info*i,const char*path,const char**index);
extern enum MHD_Result http_ret_std_file(struct http_hand_info*i,FILE*file,const char*name);
extern enum MHD_Result http_ret_file(struct http_hand_info*i,const char*path);
extern enum MHD_Result http_hand_code(struct http_hand_info*);
extern enum MHD_Result http_hand_file(struct http_hand_info*);
extern enum MHD_Result http_hand_folder(struct http_hand_info*);
extern enum MHD_Result http_hand_assets(struct http_hand_info*);
extern enum MHD_Result http_hand_assets_file(struct http_hand_info*);
extern enum MHD_Result http_hand_websocket(struct http_hand_info*);
extern enum MHD_Result http_conn_handler(void*,struct MHD_Connection*,const char*,const char*,const char*,const char*,size_t*,void**);
extern int ws_send_json_payload(struct http_hand_websocket_data*w,const char*tag,json_object*jo);
extern int ws_send_payload(struct http_hand_websocket_data*w,const char*tag,const void*payload,size_t len);
extern int ws_print_payload(struct http_hand_websocket_data*w,const char*tag,const char*payload);
extern int ws_send_cmd_r(int r,struct http_hand_websocket_data*w,const char*cmd);
extern int ws_send_cmd(struct http_hand_websocket_data*w,const char*cmd);
extern int ws_printf(struct http_hand_websocket_data*w,const char*fmt,...)__attribute__((format(printf,2,3)));
extern int ws_print(struct http_hand_websocket_data*w,const char*data);
extern int ws_write(struct http_hand_websocket_data*w,const char*data,size_t len);
extern int ws_close(struct http_hand_websocket_data*w);
#endif
