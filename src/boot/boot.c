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
		case BOOT_FOLDER:return      "Folder";
		default:return               "Unknown";
	}
}

void dump_boot_config(char*tag,enum log_level level,boot_config*boot){
	if(!tag||!boot)return;
	logger_printf(level,tag,"Dump boot config of %s",boot->ident);
	logger_printf(level,tag,"   Mode:        %s(%d)",bootmode2string(boot->mode),boot->mode);
	logger_printf(level,tag,"   Parent:      %s",boot->parent[0]?boot->parent:"(none)");
	logger_printf(level,tag,"   Description: %s",boot->desc);
	logger_printf(level,tag,"   Icon:        %s",boot->icon[0]?boot->icon:"(none)");
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
	if((buff=confd_get_string_base(cfg->base,"icon","boot"))){
		strncpy(cfg->icon,buff,sizeof(cfg->icon)-1);
		free(buff);
	}
	if((buff=confd_get_string_base(cfg->base,"parent",NULL))){
		strncpy(cfg->parent,buff,sizeof(cfg->parent)-1);
		free(buff);
	}
	cfg->mode=confd_get_integer_base(cfg->base,"mode",BOOT_NONE);
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
	logger_set_console(confd_get_boolean("boot.console_log",true));
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
