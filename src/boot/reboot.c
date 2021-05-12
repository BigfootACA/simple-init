#include<errno.h>
#include<linux/reboot.h>
#include<unistd.h>
#include"logger.h"
#include"system.h"
#include"boot.h"
#include"defines.h"
#define TAG "reboot"

int run_boot_reboot(boot_config*boot){
	if(!boot)ERET(EINVAL);
	long cmd;
	char*data=NULL;
	switch(boot->mode){
		case BOOT_REBOOT:
			data=kvarr_get(boot->data,"arg",NULL);
			cmd=data?LINUX_REBOOT_CMD_RESTART2:LINUX_REBOOT_CMD_RESTART;
			tlog_alert(data?"rebooting":"rebooting with argument '%s'",data);
		break;
		case BOOT_HALT:
			cmd=LINUX_REBOOT_CMD_HALT;
			tlog_alert("halt system");
		break;
		case BOOT_POWEROFF:
			cmd=LINUX_REBOOT_CMD_POWER_OFF;
			tlog_alert("poweroff system");
		break;
		case BOOT_KEXEC:return trlog_error(ENUM(ENOSYS),"kexec does not implemented");
		default:ERET(EINVAL);
	}
	logger_exit();
	adv_reboot(cmd,data);
	_exit(-1);
}