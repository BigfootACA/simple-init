#include<errno.h>
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

int boot(boot_config*boot){
	if(!boot||!boot->main)ERET(EINVAL);
	tlog_info("try to execute boot config %s(%s)",boot->ident,boot->desc);
	return boot->main(boot);
}