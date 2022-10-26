/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<unistd.h>
#include"str.h"
#include"xlua.h"
#include"output.h"
#include"version.h"
#include"recovery.h"
#include"filesystem.h"

static int usage(int e){
	return r_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: update-binary <1|2|3> <FD> <ZIP>\n"
		"Android recovery flash package launcher.\n"
	);
}

int update_binary_main(int argc,char**argv){
	size_t size=0;
	char buff[64];
	lua_State*lua=NULL;
	int ver,fd,r,ret=-1;
	fsh*zip=NULL,*root=NULL;
	if(argc!=4)return usage(1);
	ver=parse_int(argv[1],0);
	fd=parse_int(argv[2],-1);
	if(fd<0)return usage(2);
	if(ver!=1&&ver!=2&&ver!=3)return usage(2);
	if(access(argv[3],F_OK)!=0)return re_err(3,"access zip failed");
	recovery_out_fd=fd;
	recovery_ui_print(PRODUCT" starting");
	recovery_ui_printf("target package: %s",argv[3]);
	if((r=fs_open(NULL,&zip,argv[3],FILE_FLAG_READ))!=0)
		EDONE(recovery_ui_printf("open package %s failed: %m",argv[3]));
	if((r=fs_get_size(zip,&size))!=0)
		EDONE(recovery_ui_printf("get package %s size failed: %m",argv[3]));
	recovery_ui_printf(
		"package size: %zu bytes (%s)",
		size,make_readable_str_buf(buff,sizeof(buff),size,1,0)
	);
	if((r=fs_register_zip(zip,"pkg"))!=0)
		EDONE(recovery_ui_printf("register zip package %s failed: %m",argv[3]));
	if(!(lua=xlua_init()))
		EDONE(recovery_ui_printf("initialize lua failed"));
	recovery_ui_printf("lua initialized");
	if((r=fs_open(NULL,&root,"zip://pkg/",FILE_FLAG_FOLDER))!=0)
		EDONE(recovery_ui_printf("open package %s root failed: %m",argv[3]));
	lua_fsh_to_lua(lua,root);
	lua_setglobal(lua,"package");
	recovery_ui_printf("starting flash script...");
	if((r=xlua_run_by(lua,"updater",root,"flash.lua"))!=LUA_OK)
		EDONE(recovery_ui_printf("run lua failed: %d",r));
	recovery_ui_printf("updater done");
	ret=0;
	done:
	if(zip)fs_close(&zip);
	if(lua)lua_close(lua);
	return ret;
}