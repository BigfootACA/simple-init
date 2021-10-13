#ifdef ENABLE_GUI
#include"confd.h"
#include"gui/msgbox.h"
#include"gui/activity.h"

#ifdef ENABLE_UEFI
#include<Protocol/SimpleFileSystem.h>
extern EFI_FILE_PROTOCOL*fs_get_root_by_letter(char letter);
#endif

static bool conf_load_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	char*path=user_data;
	if(!path)return true;
	if(id==0){
		errno=0;
		#ifdef ENABLE_UEFI
		if(path[0]=='/'||path[1]!=':')return true;
		confd_load_file(fs_get_root_by_letter(*path),path+2);
		#else
		confd_load_file(path);
		#endif
		if(errno!=0)msgbox_alert("Load config failed: %s",strerror(errno));
	}
	return false;
}

static int conf_load_draw(struct gui_activity*act){
	if(!act||!act->args)return -1;
	char*path=act->args;
	#ifndef ENABLE_UEFI
	if(path[0]!='/'&&path[1]==':')path+=2;
	#endif
	msgbox_set_user_data(msgbox_create_yesno(
		conf_load_cb,
		"Load config from '%s'?",
		path
	),path);
	return -10;
}

struct gui_register guireg_conf_load={
	.name="config-load",
	.title="Load Config",
	.icon="conftool.png",
	.show_app=false,
	.open_file=true,
	.draw=conf_load_draw,
};
#endif
