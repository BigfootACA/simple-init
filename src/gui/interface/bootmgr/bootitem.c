/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"bootmgr.h"

static boot_mode bootmode[]={
	BOOT_NONE,
	BOOT_SWITCHROOT,
	BOOT_CHARGER,
	BOOT_KEXEC,
	BOOT_REBOOT,
	BOOT_POWEROFF,
	BOOT_HALT,
	BOOT_SYSTEM,
	BOOT_LINUX,
	BOOT_EFI,
	BOOT_EXIT,
	BOOT_SIMPLE_INIT,
	BOOT_UEFI_OPTION,
	BOOT_LUA,
	BOOT_FOLDER,
};

static int bootitem_get_focus(struct gui_activity*d){
	struct bootitem*bi=d->data;
	if(!bi)return 0;
	lv_group_add_obj(gui_grp,bi->txt_name);
	lv_group_add_obj(gui_grp,bi->txt_icon);
	lv_group_add_obj(gui_grp,bi->txt_desc);
	lv_group_add_obj(gui_grp,bi->sel_mode);
	lv_group_add_obj(gui_grp,bi->sel_parent);
	lv_group_add_obj(gui_grp,bi->chk_enable);
	lv_group_add_obj(gui_grp,bi->chk_show);
	lv_group_add_obj(gui_grp,bi->chk_save);
	lv_group_add_obj(gui_grp,bi->btn_extra);
	lv_group_add_obj(gui_grp,bi->btn_conf);
	lv_group_add_obj(gui_grp,bi->ok);
	lv_group_add_obj(gui_grp,bi->cancel);
	return 0;
}

static int bootitem_lost_focus(struct gui_activity*d){
	struct bootitem*bi=d->data;
	if(!bi)return 0;
	lv_group_remove_obj(bi->txt_name);
	lv_group_remove_obj(bi->txt_icon);
	lv_group_remove_obj(bi->txt_desc);
	lv_group_remove_obj(bi->sel_mode);
	lv_group_remove_obj(bi->sel_parent);
	lv_group_remove_obj(bi->chk_enable);
	lv_group_remove_obj(bi->chk_show);
	lv_group_remove_obj(bi->chk_save);
	lv_group_remove_obj(bi->btn_extra);
	lv_group_remove_obj(bi->btn_conf);
	lv_group_remove_obj(bi->ok);
	lv_group_remove_obj(bi->cancel);
	return 0;
}

static bool do_save(struct bootitem*bi){
	boot_config*c=NULL;
	uint16_t parent=lv_dropdown_get_selected(bi->sel_parent);
	uint16_t mode=lv_dropdown_get_selected(bi->sel_mode);
	const char*name=lv_textarea_get_text(bi->txt_name);
	const char*icon=lv_textarea_get_text(bi->txt_icon);
	const char*desc=lv_textarea_get_text(bi->txt_desc);
	bool enable=lv_obj_is_checked(bi->chk_enable);
	bool show=lv_obj_is_checked(bi->chk_show);
	bool save=lv_obj_is_checked(bi->chk_save);
	if(!name||!icon||!desc)return false;
	if(mode>=ARRLEN(bootmode))return false;
	if(parent>=ARRLEN(bi->folders))return false;
	if(parent>0)c=bi->folders[parent-1];
	if(!*name){
		msgbox_alert("Invalid boot item name.");
		return false;
	}
	if(bootmode[mode]==BOOT_NONE){
		msgbox_alert("Invalid boot item mode.");
		return false;
	}
	if(parent>0&&!c){
		msgbox_alert("Invalid boot item parent.");
		return false;
	}
	if(bi->name[0]&&strcmp(name,bi->name)!=0)
		confd_rename_base(bootmgr_base,bi->name,name);
	memset(bi->name,0,sizeof(bi->name));
	strncpy(bi->name,name,sizeof(bi->name)-1);
	if(!c)confd_delete_dict(bootmgr_base,bi->name,"parent");
	else confd_set_string_dict(bootmgr_base,bi->name,"parent",c->ident);
	if(!icon[0])confd_delete_dict(bootmgr_base,bi->name,"icon");
	else confd_set_string_dict(bootmgr_base,bi->name,"icon",(char*)icon);
	if(!desc[0])confd_delete_dict(bootmgr_base,bi->name,"desc");
	else confd_set_string_dict(bootmgr_base,bi->name,"desc",(char*)desc);
	switch(confd_get_type_dict(bootmgr_base,bi->name,"mode")){
		case TYPE_STRING:confd_set_string_dict(
			bootmgr_base,bi->name,"mode",
			bootmode2shortstring(bootmode[mode])
		);break;
		default:confd_set_integer_dict(
			bootmgr_base,bi->name,"mode",
			bootmode[mode]
		);break;
	}
	confd_set_boolean_dict(bootmgr_base,bi->name,"enabled",enable);
	confd_set_boolean_dict(bootmgr_base,bi->name,"show",show);
	confd_set_save_base(bootmgr_base,bi->name,save);
	bi->act->data_changed=true;
	return true;
}

static int init(struct gui_activity*act){
	struct bootitem*bi=malloc(sizeof(struct bootitem));
	if(!bi)return -ENOMEM;
	memset(bi,0,sizeof(struct bootitem));
	act->data=bi,bi->act=act;
	return 0;
}

static void load_icon(struct bootitem*bi){
	const char*img;
	lv_coord_t s=lv_obj_get_height(bi->txt_icon);
	img=lv_textarea_get_text(bi->txt_icon);
	lv_img_src_try(bi->img_icon,"bootitem",bi->name,img);
	lv_img_fill_image(bi->img_icon,s,s);
	lv_obj_center(bi->img_icon);
}

static void load_modes(struct bootitem*bi){
	int64_t mode;
	char name[BUFSIZ],*x;
	switch(confd_get_type_dict(bootmgr_base,bi->name,"mode")){
		case TYPE_INTEGER:mode=confd_get_integer_dict(
			bootmgr_base,bi->name,"mode",-1
		);break;
		case TYPE_STRING:
			if(!(x=confd_get_string_dict(
				bootmgr_base,bi->name,"mode",NULL
			)))break;
			if(!shortstring2bootmode(
				x,(enum boot_mode*)&mode
			))mode=-1;
			free(x);
		break;
		default:mode=-1;break;
	}
	lv_dropdown_clear_options(bi->sel_mode);
	for(unsigned char i=0;i<ARRLEN(bootmode);i++){
		enum boot_mode m=bootmode[i];
		memset(name,0,sizeof(name));
		snprintf(
			name,sizeof(name)-1,
			"%s (0x%02X)",
			_(bootmode2string(m)),i
		);
		lv_dropdown_add_option(bi->sel_mode,name,i);
		if(mode>=0&&mode==(int64_t)m)
			lv_dropdown_set_selected(bi->sel_mode,i);
	}
}

static void load_parents(struct bootitem*bi){
	size_t pos=0;
	boot_config*cfg=NULL;
	char name[BUFSIZ],**bs;
	memset(bi->folders,0,sizeof(bi->folders));
	lv_dropdown_clear_options(bi->sel_parent);
	lv_dropdown_add_option(bi->sel_parent,_("(none)"),0);
	if(!(bs=confd_ls(bootmgr_base)))return;
	char*parent=confd_get_string_dict(bootmgr_base,bi->name,"parent",NULL);
	for(size_t i=0;i<ARRLEN(bi->folders);i++)
		if(bi->folders[i])free(bi->folders[i]);
	memset(bi->folders,0,sizeof(bi->folders));
	for(size_t i=0;bs[i];i++){
		if(strcmp(bs[i],bi->name)==0)continue;
		if(!(cfg=boot_get_config(bs[i])))continue;
		if(cfg->mode!=BOOT_FOLDER){
			free(cfg);
			continue;
		}
		memset(name,0,sizeof(name));
		if(!cfg->desc[0])strncpy(name,cfg->ident,sizeof(name)-1);
		else snprintf(name,sizeof(name)-1,"%s (%s)",_(cfg->desc),cfg->ident);
		lv_dropdown_add_option(bi->sel_parent,name,pos+1);
		if(parent&&strcmp(cfg->ident,parent)==0)
			lv_dropdown_set_selected(bi->sel_parent,pos+1);
		bi->folders[pos++]=cfg;
	}
	if(parent)free(parent);
	if(bs[0])free(bs[0]);
	free(bs);
}

static int do_load(struct gui_activity*act){
	struct bootitem*bi=act->data;
	if(!bi)return 0;
	if(act->args){
		char*name=act->args;
		char*icon=confd_get_string_dict(bootmgr_base,name,"icon",NULL);
		char*desc=confd_get_string_dict(bootmgr_base,name,"desc",NULL);
		bool enabled=confd_get_boolean_dict(bootmgr_base,name,"enabled",true);
		bool show=confd_get_boolean_dict(bootmgr_base,name,"show",true);
		bool save=confd_get_save_base(bootmgr_base,name);
		lv_textarea_set_text(bi->txt_name,name);
		if(icon)lv_textarea_set_text(bi->txt_icon,icon);
		if(desc)lv_textarea_set_text(bi->txt_desc,desc);
		lv_obj_set_checked(bi->chk_enable,enabled);
		lv_obj_set_checked(bi->chk_show,show);
		lv_obj_set_checked(bi->chk_save,save);
		memset(bi->name,0,sizeof(bi->name));
		strncpy(bi->name,name,sizeof(bi->name)-1);
		if(icon)free(icon);
		if(desc)free(desc);
		act->args=NULL;
	}
	load_modes(bi);
	load_parents(bi);
	load_icon(bi);
	return 0;
}

static int do_cleanup(struct gui_activity*act){
	struct bootitem*bi=act->data;
	if(!bi)return 0;
	for(size_t i=0;i<ARRLEN(bi->folders);i++)
		if(bi->folders[i])free(bi->folders[i]);
	memset(bi,0,sizeof(struct bootitem));
	free(bi);
	act->data=NULL;
	return 0;
}

static void ok_cb(lv_event_t*e){
	if(do_save(e->user_data))guiact_do_back();
}

static bool arg_cb(bool ok,const char*content,void*user_data){
	struct bootitem*bi=user_data;
	if(ok)confd_set_string_dict(
		bootmgr_base,bi->name,
		"extra.arg",
		(char*)content
	);
	return true;
}

static void show_arg_edit(struct bootitem*bi){
	struct inputbox*in=inputbox_create(
		arg_cb,
		"Input reboot argument"
	);
	char*arg=confd_get_string_dict(
		bootmgr_base,bi->name,
		"extra.arg",NULL
	);
	if(arg){
		inputbox_set_content(in,"%s",arg);
		free(arg);
	}
	inputbox_set_user_data(in,bi);
}

static void extra_cb(lv_event_t*e){
	char*x;
	enum boot_mode mode=BOOT_NONE;
	struct bootitem*bi=e->user_data;
	if(!do_save(bi))return;
	switch(confd_get_type_dict(bootmgr_base,bi->name,"mode")){
		case TYPE_INTEGER:mode=confd_get_integer_dict(
			bootmgr_base,bi->name,"mode",BOOT_NONE
		);break;
		case TYPE_STRING:
			if(!(x=confd_get_string_dict(
				bootmgr_base,bi->name,"mode",NULL
			)))break;
			if(!shortstring2bootmode(x,&mode))mode=BOOT_NONE;
			free(x);
		break;
		default:mode=BOOT_NONE;break;
	}
	switch(mode){
		case BOOT_NONE:break;
		case BOOT_REBOOT:show_arg_edit(bi);break;
		case BOOT_EFI:guiact_start_activity(&guireg_bootdata_efi,bi->name);break;
		case BOOT_LINUX:guiact_start_activity(&guireg_bootdata_linux,bi->name);break;
		case BOOT_UEFI_OPTION:guiact_start_activity(&guireg_bootdata_uefi_option,bi->name);break;
		default:msgbox_alert("Unsupported boot item mode for extra data editor");
	}
}

static void conf_cb(lv_event_t*e){
	static char path[PATH_MAX];
	struct bootitem*bi=e->user_data;
	if(!do_save(bi))return;
	memset(path,0,sizeof(path));
	snprintf(path,sizeof(path)-1,"%s.%s",bootmgr_base,bi->name);
	guiact_start_activity_by_name("config-manager",path);
}

static void icon_cb(lv_event_t*e){
	load_icon(e->user_data);
}

static int draw_bootitem(struct gui_activity*act){
	struct bootitem*bi=act->data;
	bi->box=lv_draw_dialog_box(act->page,&bi->title,"Boot Item Editor");
	lv_draw_input(bi->box,"Name:",        NULL,NULL,&bi->txt_name,NULL);
	lv_draw_input(bi->box,"Icon:",        NULL,NULL,&bi->txt_icon,NULL);
	lv_draw_input(bi->box,"Description:", NULL,NULL,&bi->txt_desc,NULL);
	lv_draw_dropdown(bi->box,"Mode:",   &bi->sel_mode);
	lv_draw_dropdown(bi->box,"Parent:", &bi->sel_parent);
	lv_obj_add_event_cb(bi->txt_icon,icon_cb,LV_EVENT_DEFOCUSED,bi);
	lv_obj_t*w_img=lv_draw_wrapper(bi->box,NULL,NULL,lv_pct(100),gui_font_size*2);
	lv_obj_set_grid_cell(w_img,LV_GRID_ALIGN_STRETCH,2,1,LV_GRID_ALIGN_STRETCH,1,1);
	lv_obj_set_style_pad_all(w_img,0,0);
	bi->img_icon=lv_img_create(w_img);
	lv_img_set_size_mode(bi->img_icon,LV_IMG_SIZE_MODE_REAL);
	lv_obj_center(bi->img_icon);
	bi->chk_enable=lv_draw_checkbox(bi->box,_("Enable boot item"),true,NULL,NULL);
	bi->chk_show=lv_draw_checkbox(bi->box,_("Show in boot menu"),true,NULL,NULL);
	bi->chk_save=lv_draw_checkbox(bi->box,_("Persist boot item"),true,NULL,NULL);
	lv_draw_buttons_auto_arg(
		bi->box,
		#define BTN(tgt,title,cb,x)&(struct button_dsc){\
			&tgt,true,_(title),cb,bi,x,1,0,1,NULL\
		}
		BTN(bi->btn_extra, "Extra Items", extra_cb, 0),
		BTN(bi->btn_conf,  "Config Tool", conf_cb,  1),
		NULL
		#undef BTN
	);
	lv_draw_btns_ok_cancel(bi->box,&bi->ok,&bi->cancel,ok_cb,bi);
	return 0;
}

struct gui_register guireg_bootitem={
	.name="bootitem",
	.title="Boot Item Editor",
	.show_app=false,
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=bootitem_get_focus,
	.lost_focus=bootitem_lost_focus,
	.draw=draw_bootitem,
	.data_load=do_load,
	.back=true,
	.mask=true,
};
#endif
