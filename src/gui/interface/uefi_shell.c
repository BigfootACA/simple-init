#ifdef ENABLE_GUI
#ifdef ENABLE_UEFI
#include"stdlib.h"
#include<Uefi.h>
#include<Pi/PiFirmwareFile.h>
#include<Library/UefiLib.h>
#include<Library/DebugLib.h>
#include<Library/DevicePathLib.h>
#include<Library/DxeServicesLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/LoadedImage.h>
#include"gui.h"
#include"activity.h"
#include"language.h"
#include"logger.h"
#include"tools.h"
#define TAG "shell"
static EFI_HANDLE ih;
static int shell_get_focus(struct gui_activity*d){
	lv_group_add_msgbox(gui_grp,d->page,true);
	return 0;
}

static int shell_lost_focus(struct gui_activity*d){
	lv_group_remove_msgbox(d->page);
	return 0;
}

static int after_exit(void*d __attribute__((unused))){
	if(!ih)return -1;
	UINTN size;
	int r=0;
	if(EFI_ERROR(gBS->StartImage(ih,&size,NULL))){
		tlog_error("StartImage failed");
		r=1;
	}
	if(ih)gBS->UnloadImage(ih);
	return r;
}

static void msg_click(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_DELETE){
		guiact_do_back();
	}else if(e==LV_EVENT_VALUE_CHANGED){
		if(lv_msgbox_get_active_btn(obj)==0){
			MEDIA_FW_VOL_FILEPATH_DEVICE_PATH fn;
			EFI_LOADED_IMAGE_PROTOCOL*li;
			EFI_DEVICE_PATH_PROTOCOL*dp;
			EFI_STATUS st;
			EfiInitializeFwVolDevicepathNode(&fn,&gUefiShellFileGuid);
			st=gBS->HandleProtocol(
				gImageHandle,
				&gEfiLoadedImageProtocolGuid,
				(VOID**)&li
			);
			if(EFI_ERROR(st)){
				DebugPrint(EFI_D_ERROR,"HandleProtocol failed with %r\n",st);
				tlog_error("HandleProtocol failed");
				return;
			}
			if(!(dp=AppendDevicePathNode(
				DevicePathFromHandle(li->DeviceHandle),
				(EFI_DEVICE_PATH_PROTOCOL*)&fn
			))){
				tlog_error("AppendDevicePathNode failed");
				return;
			}
			st=gBS->LoadImage(FALSE,gImageHandle,dp,NULL,0,&ih);
			if(EFI_ERROR(st)){
				if(ih)gBS->UnloadImage(ih);
				DebugPrint(EFI_D_ERROR,"LoadImage failed: %r\n",st);
				tlog_error("LoadImage failed");
				return;
			}
			gui_run_and_exit(after_exit);
		}
		lv_msgbox_start_auto_close(obj,0);
	}
}

static int uefi_shell_draw(struct gui_activity*act){
	lv_create_yesno_msgbox(act->page,msg_click,_("Exit GUI Application and enter UEFI Shell?"));
	return 0;
}

struct gui_register guireg_uefi_shell={
	.name="uefi-shell",
	.title="UEFI Shell",
	.icon="shell.png",
	.show_app=true,
	.draw=uefi_shell_draw,
	.get_focus=shell_get_focus,
	.lost_focus=shell_lost_focus,
	.back=true,
	.mask=true,
};
#endif
#endif
