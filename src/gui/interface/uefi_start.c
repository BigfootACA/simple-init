#ifdef ENABLE_GUI
#ifdef ENABLE_UEFI
#include<stdlib.h>
#include<Uefi.h>
#include<Library/UefiLib.h>
#include<Library/DevicePathLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/LoadedImage.h>
#include"gui.h"
#include"logger.h"
#include"language.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "start"

static char full_path[PATH_MAX];
static EFI_HANDLE ih;
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

extern EFI_DEVICE_PATH_PROTOCOL*fs_get_device_path(const char*path);
static void start_cb(lv_task_t*t __attribute__((unused))){
	EFI_STATUS st;
	EFI_LOADED_IMAGE_PROTOCOL*li;
	EFI_DEVICE_PATH_PROTOCOL*p=fs_get_device_path(full_path);
	if(!p){
		msgbox_alert("get DevicePath failed");
		tlog_warn("get DevicePath failed");
		return;
	}
	st=gBS->LoadImage(FALSE,gImageHandle,p,NULL,0,&ih);
	if(EFI_ERROR(st)){
		if(ih)gBS->UnloadImage(ih);
		msgbox_alert("LoadImage failed: %llx",st);
		tlog_warn("LoadImage failed: %llx",st);
		return;
	}
	st=gBS->OpenProtocol(
		ih,&gEfiLoadedImageProtocolGuid,
		(VOID**)&li,gImageHandle,NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL
	);
	if(EFI_ERROR(st)){
		msgbox_alert("OpenProtocol failed: %llx",st);
		tlog_warn("OpenProtocol failed: %llx",st);
		return;
	}
	if(li->ImageCodeType!=EfiLoaderCode){

		if(ih)gBS->UnloadImage(ih);
		msgbox_alert("not a UEFI Application");
		tlog_warn("not a UEFI Application");
		return;
	}
	gui_run_and_exit(after_exit);
}

static bool confirm_click(uint16_t id,const char*text __attribute__((unused))){
	if(id==0)lv_task_once(lv_task_create(start_cb,1,LV_TASK_PRIO_LOWEST,NULL));
	return false;
}

void uefi_start_image(const char*path){
	memset(full_path,0,PATH_MAX-1);
	strncpy(full_path,path,PATH_MAX-1);
	msgbox_create_yesno(confirm_click,"Start UEFI Application '%s'?",path);
}
#endif
#endif
