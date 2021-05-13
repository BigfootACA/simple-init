#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include"boot.h"
#include"logger.h"
#include"defines.h"
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
	return boot->main(boot);
}