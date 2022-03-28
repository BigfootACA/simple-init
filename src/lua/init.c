/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include"xlua.h"
#include"init_internal.h"

static int send_req(lua_State*L,struct init_msg*msg,struct init_msg*res){
	errno=0;
	init_send(msg,res);
	if(res->data.status.ret!=0)
		errno=res->data.status.ret;
	lua_pushinteger(L,errno);
	return 1;
}

static int set_arg(lua_State*L,int n,bool req,char*buff,size_t size){
	if(n==0&&!req)return 0;
	const char*data=req?
		luaL_checkstring(L,n):
		luaL_optstring(L,n,NULL);
	if(!data){
		if(req)abort();
		return 0;
	}
	if(strlen(data)>=size)
		return luaL_argerror(L,n,"argument too long");
	strncpy(buff,data,size-1);
	return 0;
}
#define SET_ARG(L,n,req,buf)set_arg(L,n,req,buf,sizeof(buf));

static int single_invoke(lua_State*L,enum init_action act,int n,bool req){
	errno=0;
	struct init_msg msg,res;
	init_initialize_msg(&msg,act);
	SET_ARG(L,n,req,msg.data.data);
	return send_req(L,&msg,&res);
}

static int init_switchroot(lua_State*L){
	errno=0;
	struct init_msg msg,res;
	init_initialize_msg(&msg,ACTION_SWITCHROOT);
	SET_ARG(L,1,true,msg.data.newroot.root);
	SET_ARG(L,2,false,msg.data.newroot.init);
	return send_req(L,&msg,&res);
}

static int init_addenv(lua_State*L){
	errno=0;
	struct init_msg msg,res;
	init_initialize_msg(&msg,ACTION_ADDENV);
	SET_ARG(L,1,true,msg.data.env.key);
	SET_ARG(L,2,true,msg.data.env.value);
	return send_req(L,&msg,&res);
}

static int init_svc_stop(lua_State*L){return single_invoke(L,ACTION_SVC_STOP,1,true);}
static int init_svc_start(lua_State*L){return single_invoke(L,ACTION_SVC_START,1,true);}
static int init_svc_restart(lua_State*L){return single_invoke(L,ACTION_SVC_RESTART,1,true);}
static int init_svc_reload(lua_State*L){return single_invoke(L,ACTION_SVC_RELOAD,1,true);}
static int init_language(lua_State*L){return single_invoke(L,ACTION_LANGUAGE,1,true);}
static int init_reboot(lua_State*L){return single_invoke(L,ACTION_REBOOT,1,false);}
static int init_poweroff(lua_State*L){return single_invoke(L,ACTION_POWEROFF,0,false);}
static int init_halt(lua_State*L){return single_invoke(L,ACTION_HALT,0,false);}

static const luaL_Reg init_lib[]={
	{"halt",            init_halt},
	{"reboot",          init_reboot},
	{"poweroff",        init_poweroff},
	{"set_lang",        init_language},
	{"set_language",    init_language},
	{"svc_stop",        init_svc_stop},
	{"service_stop",    init_svc_stop},
	{"svc_start",       init_svc_start},
	{"service_start",   init_svc_start},
	{"svc_restart",     init_svc_restart},
	{"service_restart", init_svc_restart},
	{"svc_reload",      init_svc_reload},
	{"service_reload",  init_svc_reload},
	{"switchroot",      init_switchroot},
	{"addenv",          init_addenv},
	{"setenv",          init_addenv},
	{NULL, NULL}
};

LUAMOD_API int luaopen_init(lua_State*L){
	luaL_newlib(L,init_lib);
	open_socket_initfd(DEFAULT_INITD,true);
	return 1;
}
#endif
