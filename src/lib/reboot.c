#define _GNU_SOURCE
#include<errno.h>
#include<unistd.h>
#include<syscall.h>
#include<sys/reboot.h>
#include<linux/reboot.h>
#include"defines.h"

int adv_reboot(long cmd,char*data){
	sync();
	switch(cmd){
		case LINUX_REBOOT_CMD_HALT:
		case LINUX_REBOOT_CMD_KEXEC:
		case LINUX_REBOOT_CMD_POWER_OFF:
		case LINUX_REBOOT_CMD_RESTART:
		case LINUX_REBOOT_CMD_SW_SUSPEND:
			return reboot((int)cmd);
		case LINUX_REBOOT_CMD_RESTART2:
			if(data)return (int)syscall(
				SYS_reboot,
				LINUX_REBOOT_MAGIC1,
				LINUX_REBOOT_MAGIC2,
				LINUX_REBOOT_CMD_RESTART2,
				data
			);
		//fallthrough
		default:ERET(EINVAL);
	}
}