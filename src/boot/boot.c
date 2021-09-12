#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include"boot.h"
#include"logger.h"
#include"defines.h"
#include"cmdline.h"
#include"service.h"
#include"init_internal.h"
#define TAG "boot"

char*bootmode2string(enum boot_mode mode){
	switch (mode){
		case BOOT_NONE:return       "Normal";
		case BOOT_SWITCHROOT:return "Switch Root";
		case BOOT_CHARGER:return    "Charger";
		case BOOT_KEXEC:return      "KEXEC";
		case BOOT_REBOOT:return     "Reboot";
		case BOOT_POWEROFF:return   "Power Off";
		case BOOT_HALT:return       "Halt";
		default:return              "Unknown";
	}
}

void dump_boot_config(char*tag,enum log_level level,boot_config*boot){
	if(!tag||!boot)return;
	logger_printf(level,tag,"Dump boot config of %s",boot->ident);
	logger_printf(level,tag,"   Mode:        %s(%d)",bootmode2string(boot->mode),boot->mode);
	logger_printf(level,tag,"   Description: %s",boot->desc);
	logger_printf(level,tag,"   Enter point: %p",boot->main);
	logger_printf(level,tag,"   Extra data: ");
	char*buff;
	KVARR_FOREACH(boot->data,k,i){
		size_t s=4+(k->key?strlen(k->key):6)+(k->value?strlen(k->value):6);
		if(!(buff=malloc(s)))break;
		memset(buff,0,s);
		kv_print(k,buff,s," = ");
		logger_printf(level,tag,"      %s",buff);
		free(buff);
	}
}

int boot(boot_config*boot){
	if(!boot||!boot->main)ERET(EINVAL);
	tlog_info("try to execute boot config %s(%s)",boot->ident,boot->desc);
	int r=boot->main(boot);
	if(r!=0){
		telog_error("execute boot config %s(%s) failed with %d",boot->ident,boot->desc,r);
		dump_boot_config("bootconfig",LEVEL_NOTICE,boot);
	}
	return r;
}

static int default_boot(struct service*svc __attribute__((unused))){
	open_socket_initfd(DEFAULT_INITD,false);
	open_socket_logfd_default();
	if(boot(boot_options.config)!=0){
		errno=0;
		struct init_msg msg,response;
		init_initialize_msg(&msg,ACTION_SVC_START);
		strcpy(msg.data.data,"system");
		init_send(&msg,&response);
		if(errno!=0)telog_error("send service command failed");
		errno=response.data.status.ret;
		if(errno!=0)telog_error("start system failed");
		return response.data.status.ret;
	}
	return 0;
}

int register_default_boot(){
	struct service*svc_boot=svc_create_service("default-boot",WORK_ONCE);
	if(svc_boot){
		svc_set_desc(svc_boot,"Boot into System");
		svc_set_start_function(svc_boot,default_boot);
		svc_boot->stop_on_shutdown=false;
		svc_add_depend(svc_default,svc_boot);
	}
	return 0;
}
