/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"boot.h"
#include"confd.h"
#include"logger.h"
#include"xlua.h"
#define TAG "boot-lua"

int run_boot_lua(boot_config*boot){
	int r=-1;
	#ifdef ENABLE_LUA
	char*file=NULL;
	lua_State*st=NULL;
	if(!(file=confd_get_string_base(boot->key,"file",NULL)))
		EDONE(tlog_error("lua file path not set"));
	if(!(st=xlua_init()))
		EDONE(tlog_error("init lua context failed"));
	lua_pushinteger(st,0);
	lua_setglobal(st,"retval");
	boot_config_to_lua(st,boot);
	lua_setglobal(st,"boot");
	if((r=xlua_run(st,TAG,file))==LUA_OK){
		lua_getglobal(st,"retval");
		r=lua_tointeger(st,-1);
	}
	done:
	if(file)free(file);
	if(st)lua_close(st);
	#endif
	return r;
}
