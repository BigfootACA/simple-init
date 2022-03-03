/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<unistd.h>
#ifdef ENABLE_UEFI
#include<Uefi.h>
#include<Library/BaseLib.h>
#include<Library/UefiRuntimeServicesTableLib.h>
#else
#include<syscall.h>
#include<sys/reboot.h>
#include<linux/reboot.h>
#endif
#include"system.h"
#include"defines.h"

#ifdef ENABLE_UEFI
typedef enum REBOOT_REASON_TYPE{
	NORMAL_MODE=0x0,
	RECOVERY_MODE=0x1,
	FASTBOOT_MODE=0x2,
	ALARM_BOOT=0x3,
	DM_VERITY_LOGGING=0x4,
	DM_VERITY_ENFORCING=0x5,
	DM_VERITY_KEYSCLEAR=0x6,
	OEM_RESET_MIN=0x20,
	OEM_RESET_MAX=0x3f,
	EMERGENCY_DLOAD=0xFF,
}REBOOT_REASON_TYPE;

typedef struct RESET_DATA_TYPE{
	CHAR16 DataBuffer[12];
	UINT8 Bdata;
}RESET_DATA_TYPE;

int adv_reboot(enum reboot_cmd cmd,char*data){
	UINTN s=0;
	VOID*x=NULL;
	RESET_DATA_TYPE rdt;
	EFI_RESET_TYPE t=EfiResetPlatformSpecific;
	EFI_STATUS rt=EFI_SUCCESS;
	rdt.Bdata=0;
	StrCpyS(rdt.DataBuffer,sizeof(rdt.DataBuffer),L"RESET_PARAM");
	switch(cmd){
		case REBOOT_HALT:t=EfiResetShutdown;break;
		case REBOOT_POWEROFF:t=EfiResetShutdown;break;
		case REBOOT_RESTART:
		case REBOOT_COLD:t=EfiResetCold;break;
		case REBOOT_WARM:t=EfiResetWarm;break;
		case REBOOT_RECOVERY:rdt.Bdata=RECOVERY_MODE;break;
		case REBOOT_FASTBOOT:rdt.Bdata=FASTBOOT_MODE;break;
		case REBOOT_EDL:s=6,x=L"EDL",rt=EFI_INVALID_PARAMETER;break;
		case REBOOT_DATA:
			if(data){
				if(AsciiStriCmp(data,"bootloader")==0)
					return adv_reboot(REBOOT_FASTBOOT,NULL);
				else if(AsciiStriCmp(data,"fastboot")==0)
					return adv_reboot(REBOOT_FASTBOOT,NULL);
				else if(AsciiStriCmp(data,"recovery")==0)
					return adv_reboot(REBOOT_RECOVERY,NULL);
				else if(AsciiStriCmp(data,"edl")==0)
					return adv_reboot(REBOOT_EDL,NULL);
				s=AsciiStrLen(data),x=data;
				break;
			}
			//fallthrough
		default:ERET(EINVAL);
	}
	if(rdt.Bdata!=0)t=EfiResetCold,x=&rdt,s=sizeof(rdt);
	gRT->ResetSystem(t,rt,s,x);
	ERET(EINVAL);
}

#else

int adv_reboot(enum reboot_cmd cmd,char*data){
	sync();
	int oper=0;
	switch(cmd){
		case REBOOT_HALT:oper=LINUX_REBOOT_CMD_HALT;break;
		case REBOOT_KEXEC:oper=LINUX_REBOOT_CMD_KEXEC;break;
		case REBOOT_POWEROFF:oper=LINUX_REBOOT_CMD_POWER_OFF;break;
		case REBOOT_COLD:oper=LINUX_REBOOT_CMD_RESTART;break;
		case REBOOT_WARM:oper=LINUX_REBOOT_CMD_RESTART;break;
		case REBOOT_RESTART:oper=LINUX_REBOOT_CMD_RESTART;break;
		case REBOOT_SUSPEND:oper=LINUX_REBOOT_CMD_SW_SUSPEND;break;
		case REBOOT_RECOVERY:return adv_reboot(REBOOT_DATA,"recovery");
		case REBOOT_FASTBOOT:return adv_reboot(REBOOT_DATA,"bootloader");
		case REBOOT_EDL:return adv_reboot(REBOOT_DATA,"edl");
		case REBOOT_DATA:oper=LINUX_REBOOT_CMD_RESTART2;break;
		default:ERET(EINVAL);
	}
	if(oper!=(int)LINUX_REBOOT_CMD_RESTART2)return reboot(oper);
	else if(!data)ERET(EINVAL);
	else return (int)syscall(
		SYS_reboot,
		LINUX_REBOOT_MAGIC1,
		LINUX_REBOOT_MAGIC2,
		oper,data
	);
}
#endif
