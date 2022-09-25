/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>
#include"str.h"
#include"boot.h"
#include"confd.h"
#include"logger.h"
#include"defines.h"
#ifdef ENABLE_UEFI
#include<Library/UefiBootServicesTableLib.h>
#else
#include"service.h"
#include"init_internal.h"
#endif
#define TAG "boot"

static const char*base="boot.configs";

char*bootmode2string(enum boot_mode mode){
	switch(mode){
		case BOOT_NONE:return        "Normal";
		case BOOT_SWITCHROOT:return  "Switch Root";
		case BOOT_CHARGER:return     "Charger";
		case BOOT_KEXEC:return       "KEXEC";
		case BOOT_REBOOT:return      "Reboot";
		case BOOT_POWEROFF:return    "Power Off";
		case BOOT_HALT:return        "Halt";
		case BOOT_SYSTEM:return      "System";
		case BOOT_LINUX:return       "Boot Linux";
		case BOOT_EFI:return         "UEFI App";
		case BOOT_EXIT:return        "Continue Boot";
		case BOOT_SIMPLE_INIT:return "Simple Init";
		case BOOT_UEFI_OPTION:return "UEFI Boot Option";
		case BOOT_LUA:return         "Lua Script";
		case BOOT_FOLDER:return      "Folder";
		default:return               "Unknown";
	}
}

char*bootmode2shortstring(enum boot_mode mode){
	switch(mode){
		case BOOT_SWITCHROOT:return  "switchroot";
		case BOOT_CHARGER:return     "charger";
		case BOOT_KEXEC:return       "kexec";
		case BOOT_REBOOT:return      "reboot";
		case BOOT_POWEROFF:return    "poweroff";
		case BOOT_HALT:return        "halt";
		case BOOT_SYSTEM:return      "system";
		case BOOT_LINUX:return       "linux";
		case BOOT_EFI:return         "efi";
		case BOOT_EXIT:return        "exit";
		case BOOT_SIMPLE_INIT:return "simple-init";
		case BOOT_UEFI_OPTION:return "uefi-option";
		case BOOT_LUA:return         "lua";
		case BOOT_FOLDER:return      "folder";
		default:return               "unknown";
	}
}

bool shortstring2bootmode(const char*str,enum boot_mode*mode){
	if(!str||!mode)return false;
	else if(strcasecmp(str,"switchroot")==0)*mode=BOOT_SWITCHROOT;
	else if(strcasecmp(str,"charger")==0)*mode=BOOT_CHARGER;
	else if(strcasecmp(str,"kexec")==0)*mode=BOOT_KEXEC;
	else if(strcasecmp(str,"reboot")==0)*mode=BOOT_REBOOT;
	else if(strcasecmp(str,"poweroff")==0)*mode=BOOT_POWEROFF;
	else if(strcasecmp(str,"halt")==0)*mode=BOOT_HALT;
	else if(strcasecmp(str,"system")==0)*mode=BOOT_SYSTEM;
	else if(strcasecmp(str,"linux")==0)*mode=BOOT_LINUX;
	else if(strcasecmp(str,"efi")==0)*mode=BOOT_EFI;
	else if(strcasecmp(str,"exit")==0)*mode=BOOT_EXIT;
	else if(strcasecmp(str,"simple-init")==0)*mode=BOOT_SIMPLE_INIT;
	else if(strcasecmp(str,"uefi-option")==0)*mode=BOOT_UEFI_OPTION;
	else if(strcasecmp(str,"lua")==0)*mode=BOOT_LUA;
	else if(strcasecmp(str,"folder")==0)*mode=BOOT_FOLDER;
	else return false;
	return true;
}

void dump_boot_config(char*tag,enum log_level level,boot_config*boot){
	if(!tag||!boot)return;
	logger_printf(level,tag,"Dump boot config of %s",boot->ident);
	logger_printf(level,tag,"   Mode:        %s(%d)",bootmode2string(boot->mode),boot->mode);
	logger_printf(level,tag,"   Parent:      %s",boot->parent[0]?boot->parent:"(none)");
	logger_printf(level,tag,"   Description: %s",boot->desc);
	logger_printf(level,tag,"   Icon:        %s",boot->icon[0]?boot->icon:"(none)");
	logger_printf(level,tag,"   Splash:      %s",boot->splash[0]?boot->splash:"(none)");
	logger_printf(level,tag,"   Show:        %s",BOOL2STR(boot->show));
	logger_printf(level,tag,"   Enabled:     %s",BOOL2STR(boot->enabled));
	logger_printf(level,tag,"   Extra data: ");
	char**ds=confd_ls(boot->key),*val,buff[PATH_MAX];
	if(!ds)return;
	for(size_t i=0;ds[i];i++){
		memset(buff,0,sizeof(buff));
		switch(confd_get_type_base(boot->key,ds[i])){
			case TYPE_STRING:
				val=confd_get_string_base(boot->key,ds[i],NULL);
				if(!val)strcpy(buff,"(NULL)");
				else{
					strncpy(buff,val,sizeof(buff)-1);
					free(val);
				}
			break;
			case TYPE_INTEGER:snprintf(
				buff,sizeof(buff)-1,"%lld",
				(long long int)confd_get_integer_base(boot->key,ds[i],0)
			);break;
			case TYPE_BOOLEAN:strcpy(
				buff,BOOL2STR(confd_get_boolean_base(boot->key,ds[i],false))
			);break;
			default:continue;
		}
		logger_printf(level,tag,"      %s = %s",ds[i],buff);
	}
	if(ds[0])free(ds[0]);
	if(ds)free(ds);
}

int boot_create_config(struct boot_config*cfg,keyval**data){
	memset(cfg->key,0,sizeof(cfg->key));
	snprintf(cfg->base,sizeof(cfg->base)-1,"%s.%s",base,cfg->ident);
	snprintf(cfg->key,sizeof(cfg->key)-1,"%s.extra",cfg->base);
	if(confd_get_type(cfg->base)==TYPE_KEY&&!cfg->replace)goto done;
	confd_add_key(cfg->base);
	confd_set_save(cfg->base,cfg->save);
	confd_set_integer_base(cfg->base,"mode",cfg->mode);
	confd_set_string_base(cfg->base,"desc",(char*)(cfg->desc[0]?cfg->desc:cfg->ident));
	confd_set_boolean_base(cfg->base,"show",cfg->show);
	confd_set_boolean_base(cfg->base,"enabled",cfg->enabled);
	if(cfg->parent[0])confd_set_string_base(cfg->base,"parent",(char*)cfg->parent);
	if(cfg->icon[0])confd_set_string_base(cfg->base,"icon",(char*)cfg->icon);
	if(cfg->splash[0])confd_set_string_base(cfg->base,"splash",(char*)cfg->splash);
	if(data)for(size_t i=0;data[i];i++)
		confd_set_string_base(cfg->key,data[i]->key,data[i]->value);
	done:
	return 0;
}

boot_config*boot_get_config(const char*name){
	char*n=(char*)name,*x=NULL;
	if(!n)n=x=confd_get_string("boot.current",NULL);
	if(!n)n=x=confd_get_string("boot.default",NULL);
	if(confd_get_type_base(base,n)!=TYPE_KEY)return NULL;
	boot_config*cfg=malloc(sizeof(boot_config));
	if(!cfg)EPRET(ENOMEM);
	memset(cfg,0,sizeof(boot_config));
	snprintf(cfg->base,sizeof(cfg->base)-1,"%s.%s",base,n);
	snprintf(cfg->key,sizeof(cfg->key)-1,"%s.extra",cfg->base);
	char*buff;
	strncpy(cfg->ident,n,sizeof(cfg->ident)-1);
	if((buff=confd_get_string_base(cfg->base,"desc",n))){
		strncpy(cfg->desc,buff,sizeof(cfg->desc)-1);
		free(buff);
	}
	if((buff=confd_get_string_base(cfg->base,"icon",NULL))){
		strncpy(cfg->icon,buff,sizeof(cfg->icon)-1);
		free(buff);
	}
	if((buff=confd_get_string_base(cfg->base,"splash",NULL))){
		strncpy(cfg->splash,buff,sizeof(cfg->splash)-1);
		free(buff);
	}
	if((buff=confd_get_string_base(cfg->base,"parent",NULL))){
		strncpy(cfg->parent,buff,sizeof(cfg->parent)-1);
		free(buff);
	}
	cfg->mode=BOOT_NONE;
	switch(confd_get_type_base(cfg->base,"mode")){
		case TYPE_INTEGER:cfg->mode=confd_get_integer_base(cfg->base,"mode",BOOT_NONE);break;
		case TYPE_STRING:{
			char*k=confd_get_string_base(cfg->base,"mode",NULL);
			if(!k)break;
			if(!shortstring2bootmode(k,&cfg->mode))
				tlog_warn("unknown boot mode: %s",x);
			free(k);
		}break;
		default:tlog_warn("unknown boot mode");break;
	}
	cfg->show=confd_get_boolean_base(cfg->base,"show",false);
	cfg->enabled=confd_get_boolean_base(cfg->base,"enabled",false);
	cfg->save=confd_get_save(cfg->base);
	if(x)free(x);
	return cfg;
}

int boot(boot_config*boot){
	boot_config*x=NULL;
	if(!boot)x=boot=boot_get_config(NULL);
	if(!boot)return -1;
	#ifdef ENABLE_UEFI
	gST->ConOut->ClearScreen(gST->ConOut);
	logger_set_console(confd_get_boolean(
		"boot.console_log",
		!boot->splash[0]
	));
	if(boot->splash[0])boot_draw_splash(boot);
	#endif
	tlog_info("try to execute boot config %s(%s)",boot->ident,boot->desc);
	boot_main*main=boot_main_func[boot->mode];
	if(!main){
		tlog_error("boot config %s(%s) not supported",boot->ident,boot->desc);
		dump_boot_config("bootconfig",LEVEL_NOTICE,boot);
		if(x)free(x);
		ERET(ENOTSUP);
	}
	int r=main(boot);
	if(r!=0){
		telog_error("execute boot config %s(%s) failed with %d",boot->ident,boot->desc,r);
		dump_boot_config("bootconfig",LEVEL_NOTICE,boot);
	}
	#ifdef ENABLE_UEFI
	gBS->Stall(10*1000*1000);
	logger_set_console(false);
	gST->ConOut->ClearScreen(gST->ConOut);
	#endif
	if(x)free(x);
	return r;
}

int boot_name(const char*name){
	boot_config*cfg=boot_get_config(name);
	if(!cfg)return -1;
	int r=boot(cfg);
	free(cfg);
	return r;
}

#ifndef ENABLE_UEFI
static int default_boot(struct service*svc __attribute__((unused))){
	open_socket_initfd(DEFAULT_INITD,false);
	open_socket_logfd_default();
	open_default_confd_socket(false,TAG);
	int r;
	if((r=boot(NULL))==0)return 0;
	char*sec=confd_get_string("boot.second",NULL);
	if(sec){
		tlog_notice("try second config %s",sec);
		r=boot_name(sec);
		free(sec);
	}
	return r;
}

int register_default_boot(){
	struct service*svc_boot=svc_create_service("default-boot",WORK_ONCE);
	if(svc_boot){
		svc_set_desc(svc_boot,"Boot into System");
		svc_set_start_function(svc_boot,default_boot);
		svc_boot->stop_on_shutdown=false;
	}
	return 0;
}
#endif

#ifdef ENABLE_LUA
int boot_config_to_lua(lua_State*st,boot_config*boot){
	lua_createtable(st,0,0);
	lua_pushliteral(st,"mode");
	lua_pushinteger(st,boot->mode);
	lua_settable(st,-3);
	lua_pushliteral(st,"ident");
	lua_pushstring(st,boot->ident);
	lua_settable(st,-3);
	lua_pushliteral(st,"parent");
	lua_pushstring(st,boot->parent);
	lua_settable(st,-3);
	lua_pushliteral(st,"icon");
	lua_pushstring(st,boot->icon);
	lua_settable(st,-3);
	lua_pushliteral(st,"desc");
	lua_pushstring(st,boot->desc);
	lua_settable(st,-3);
	lua_pushliteral(st,"base");
	lua_pushstring(st,boot->base);
	lua_settable(st,-3);
	lua_pushliteral(st,"key");
	lua_pushstring(st,boot->key);
	lua_settable(st,-3);
	lua_pushliteral(st,"save");
	lua_pushboolean(st,boot->save);
	lua_settable(st,-3);
	lua_pushliteral(st,"replace");
	lua_pushboolean(st,boot->replace);
	lua_settable(st,-3);
	lua_pushliteral(st,"show");
	lua_pushboolean(st,boot->show);
	lua_settable(st,-3);
	lua_pushliteral(st,"enabled");
	lua_pushboolean(st,boot->enabled);
	lua_settable(st,-3);
	return 0;
}
#endif
