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
	bool enable=lv_checkbox_is_checked(bi->chk_enable);
	bool show=lv_checkbox_is_checked(bi->chk_show);
	bool save=lv_checkbox_is_checked(bi->chk_save);
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
	lv_coord_t s=lv_obj_get_height(bi->txt_icon);
	lv_img_ext_t*ext=lv_obj_get_ext_attr(bi->img_icon);
	lv_img_set_src(bi->img_icon,lv_textarea_get_text(bi->txt_icon));
	if((ext->w<=0||ext->h<=0))lv_img_set_src(bi->img_icon,"apps.svg");
	lv_img_fill_image(bi->img_icon,s,s);
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
		lv_checkbox_set_checked(bi->chk_enable,enabled);
		lv_checkbox_set_checked(bi->chk_show,show);
		lv_checkbox_set_checked(bi->chk_save,save);
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

static void ok_cb(lv_obj_t*obj,lv_event_t e){
	struct bootitem*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->ok||e!=LV_EVENT_CLICKED)return;
	if(do_save(bi))guiact_do_back();
}

static void cancel_cb(lv_obj_t*obj,lv_event_t e){
	struct bootitem*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->cancel||e!=LV_EVENT_CLICKED)return;
	guiact_do_back();
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

static void extra_cb(lv_obj_t*obj,lv_event_t e){
	char*x;
	enum boot_mode mode=BOOT_NONE;
	struct bootitem*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->btn_extra||e!=LV_EVENT_CLICKED)return;
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

static void conf_cb(lv_obj_t*obj,lv_event_t e){
	static char path[PATH_MAX];
	struct bootitem*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->btn_conf||e!=LV_EVENT_CLICKED)return;
	if(!do_save(bi))return;
	memset(path,0,sizeof(path));
	snprintf(path,sizeof(path)-1,"%s.%s",bootmgr_base,bi->name);
	guiact_start_activity_by_name("config-manager",path);
}

static void icon_cb(lv_obj_t*obj,lv_event_t e){
	struct bootitem*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->txt_icon)return;
	lv_input_cb(obj,e);
	if(e==LV_EVENT_DEFOCUSED)
		load_icon(bi);
}

static int do_resize(struct gui_activity*act){
	lv_coord_t h=0,w=act->w/8*7,x=0,s;
	struct bootitem*bi=act->data;
	if(!bi)return 0;
	lv_obj_set_style_local_pad_all(
		bi->box,
		LV_PAGE_PART_BG,
		LV_STATE_DEFAULT,
		gui_font_size
	);
	lv_obj_set_width(bi->page,w);
	w=lv_page_get_scrl_width(bi->page);
	lv_obj_set_width(bi->box,w);

	lv_obj_set_width(bi->title,w);
	lv_obj_set_pos(bi->title,x,h);
	lv_label_set_align(
		bi->title,
		LV_LABEL_ALIGN_CENTER
	);
	h+=lv_obj_get_height(bi->title);

	h+=gui_font_size/2;
	lv_obj_set_pos(bi->lbl_name,x,h);
	lv_obj_align(
		bi->txt_name,bi->lbl_name,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/2,0
	);
	lv_obj_set_y(bi->txt_name,h);
	lv_obj_align(
		bi->lbl_name,bi->txt_name,
		LV_ALIGN_OUT_LEFT_MID,
		-gui_font_size/2,0
	);
	lv_obj_set_width(
		bi->txt_name,w-
		lv_obj_get_width(bi->lbl_name)-
		gui_font_size
	);
	h+=lv_obj_get_height(bi->txt_name);

	h+=gui_font_size/2;
	lv_obj_set_pos(bi->lbl_icon,x,h);
	lv_obj_align(
		bi->img_icon,bi->lbl_icon,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/2,0
	);
	lv_obj_set_y(bi->img_icon,h);
	lv_obj_align(
		bi->lbl_icon,bi->img_icon,
		LV_ALIGN_OUT_LEFT_MID,
		-gui_font_size/2,0
	);
	s=lv_obj_get_height(bi->txt_icon);
	lv_obj_set_width(bi->img_icon,s);
	lv_obj_set_height(bi->img_icon,s);
	h+=s+gui_font_size/2;

	lv_obj_set_width(
		bi->txt_icon,w-s-
		lv_obj_get_width(bi->lbl_icon)-
		(gui_font_size/2*3)
	);
	lv_obj_align(
		bi->txt_icon,bi->img_icon,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/2,0
	);

	lv_obj_set_pos(bi->lbl_desc,x,h);
	h+=lv_obj_get_height(bi->lbl_desc);

	lv_obj_set_pos(bi->txt_desc,x,h);
	lv_obj_set_width(bi->txt_desc,w);
	h+=lv_obj_get_height(bi->txt_desc);

	h+=gui_font_size/2;
	lv_obj_set_pos(bi->lbl_mode,x,h);
	lv_obj_align(
		bi->sel_mode,bi->lbl_mode,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/2,0
	);
	lv_obj_set_y(bi->sel_mode,h);
	lv_obj_align(
		bi->lbl_mode,bi->sel_mode,
		LV_ALIGN_OUT_LEFT_MID,
		-gui_font_size/2,0
	);
	lv_obj_set_width(
		bi->sel_mode,w-
		lv_obj_get_width(bi->lbl_mode)-
		gui_font_size
	);
	h+=lv_obj_get_height(bi->sel_mode);

	h+=gui_font_size/2;
	lv_obj_set_pos(bi->lbl_parent,x,h);
	lv_obj_align(
		bi->sel_parent,bi->lbl_parent,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/2,0
	);
	lv_obj_set_y(bi->sel_parent,h);
	lv_obj_align(
		bi->lbl_parent,bi->sel_parent,
		LV_ALIGN_OUT_LEFT_MID,
		-gui_font_size/2,0
	);
	lv_obj_set_width(
		bi->sel_parent,w-
		lv_obj_get_width(bi->lbl_parent)-
		gui_font_size
	);
	h+=lv_obj_get_height(bi->sel_parent);

	h+=gui_font_size/2;
	lv_obj_set_pos(bi->chk_enable,x,h);
	h+=lv_obj_get_height(bi->chk_enable);

	h+=gui_font_size/2;
	lv_obj_set_pos(bi->chk_show,x,h);
	h+=lv_obj_get_height(bi->chk_show);

	h+=gui_font_size/2;
	lv_obj_set_pos(bi->chk_save,x,h);
	h+=lv_obj_get_height(bi->chk_save);

	h+=gui_font_size;
	lv_obj_set_size(
		bi->btn_extra,
		w/2-gui_font_size,
		gui_font_size*2
	);
	lv_obj_align(
		bi->btn_extra,NULL,
		LV_ALIGN_IN_TOP_LEFT,
		(x+(gui_font_size/2)),h
	);

	lv_obj_set_size(
		bi->btn_conf,
		w/2-gui_font_size,
		gui_font_size*2
	);
	lv_obj_align(
		bi->btn_conf,NULL,
		LV_ALIGN_IN_TOP_RIGHT,
		-(x+(gui_font_size/2)),h
	);
	h+=lv_obj_get_height(bi->btn_conf);

	h+=gui_font_size/2;
	lv_obj_set_size(
		bi->ok,
		w/2-gui_font_size,
		gui_font_size*2
	);
	lv_obj_align(
		bi->ok,NULL,
		LV_ALIGN_IN_TOP_LEFT,
		(x+(gui_font_size/2)),h
	);

	lv_obj_set_size(
		bi->cancel,
		w/2-gui_font_size,
		gui_font_size*2
	);
	lv_obj_align(
		bi->cancel,NULL,
		LV_ALIGN_IN_TOP_RIGHT,
		-(x+(gui_font_size/2)),h
	);
	h+=lv_obj_get_height(bi->cancel);

	h+=gui_font_size/2;
	lv_obj_set_height(bi->box,h);
	h+=gui_font_size*2;
	lv_obj_set_height(bi->page,MIN(h,(lv_coord_t)gui_sh/6*5));
	lv_obj_align(bi->page,NULL,LV_ALIGN_CENTER,0,0);
	return 0;
}

static int draw_bootitem(struct gui_activity*act){
	struct bootitem*bi=act->data;

	bi->page=lv_page_create(act->page,NULL);
	lv_obj_set_click(bi->page,false);

	bi->box=lv_obj_create(bi->page,NULL);
	lv_obj_set_click(bi->box,false);
	lv_obj_set_style_local_border_width(bi->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);

	// Title
	bi->title=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->title,_("Boot Item Editor"));
	lv_label_set_long_mode(bi->title,LV_LABEL_LONG_BREAK);

	bi->lbl_name=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_name,_("Name:"));

	bi->txt_name=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_name,"");
	lv_textarea_set_one_line(bi->txt_name,true);
	lv_textarea_set_cursor_hidden(bi->txt_name,true);
	lv_obj_set_user_data(bi->txt_name,bi);
	lv_obj_set_event_cb(bi->txt_name,lv_input_cb);

	bi->lbl_icon=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_icon,_("Icon:"));

	bi->txt_icon=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_icon,"");
	lv_textarea_set_one_line(bi->txt_icon,true);
	lv_textarea_set_cursor_hidden(bi->txt_icon,true);
	lv_obj_set_user_data(bi->txt_icon,bi);
	lv_obj_set_event_cb(bi->txt_icon,icon_cb);

	bi->img_icon=lv_img_create(bi->box,NULL);

	bi->lbl_desc=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_desc,_("Description:"));

	bi->txt_desc=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_desc,"");
	lv_textarea_set_one_line(bi->txt_desc,true);
	lv_textarea_set_cursor_hidden(bi->txt_desc,true);
	lv_obj_set_user_data(bi->txt_desc,bi);
	lv_obj_set_event_cb(bi->txt_desc,lv_input_cb);

	bi->lbl_mode=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_mode,_("Mode:"));

	bi->sel_mode=lv_dropdown_create(bi->box,NULL);
	lv_obj_set_user_data(bi->sel_mode,bi);
	lv_obj_set_event_cb(bi->sel_mode,lv_default_dropdown_cb);

	bi->lbl_parent=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_parent,_("Parent:"));

	bi->sel_parent=lv_dropdown_create(bi->box,NULL);
	lv_obj_set_user_data(bi->sel_parent,bi);
	lv_obj_set_event_cb(bi->sel_parent,lv_default_dropdown_cb);

	bi->chk_enable=lv_checkbox_create(bi->box,NULL);
	lv_obj_set_user_data(bi->chk_enable,bi);
	lv_checkbox_set_text(bi->chk_enable,_("Enable boot item"));
	lv_checkbox_set_checked(bi->chk_enable,true);
	lv_style_set_focus_checkbox(bi->chk_enable);

	bi->chk_show=lv_checkbox_create(bi->box,NULL);
	lv_obj_set_user_data(bi->chk_show,bi);
	lv_checkbox_set_text(bi->chk_show,_("Show in boot menu"));
	lv_checkbox_set_checked(bi->chk_show,true);
	lv_style_set_focus_checkbox(bi->chk_show);

	bi->chk_save=lv_checkbox_create(bi->box,NULL);
	lv_obj_set_user_data(bi->chk_save,bi);
	lv_checkbox_set_text(bi->chk_save,_("Persist boot item"));
	lv_checkbox_set_checked(bi->chk_save,true);
	lv_style_set_focus_checkbox(bi->chk_save);

	// Extra Data Editor
	bi->btn_extra=lv_btn_create(bi->box,NULL);
	lv_style_set_action_button(bi->btn_extra,true);
	lv_obj_set_user_data(bi->btn_extra,bi);
	lv_obj_set_event_cb(bi->btn_extra,extra_cb);
	lv_label_set_text(lv_label_create(bi->btn_extra,NULL),_("Extra Items"));

	// Open via Config Tool
	bi->btn_conf=lv_btn_create(bi->box,NULL);
	lv_style_set_action_button(bi->btn_conf,true);
	lv_obj_set_user_data(bi->btn_conf,bi);
	lv_obj_set_event_cb(bi->btn_conf,conf_cb);
	lv_label_set_text(lv_label_create(bi->btn_conf,NULL),_("Config Tool"));

	// OK Button
	bi->ok=lv_btn_create(bi->box,NULL);
	lv_style_set_action_button(bi->ok,true);
	lv_obj_set_user_data(bi->ok,bi);
	lv_obj_set_event_cb(bi->ok,ok_cb);
	lv_label_set_text(lv_label_create(bi->ok,NULL),_("OK"));

	// Cancel Button
	bi->cancel=lv_btn_create(bi->box,NULL);
	lv_style_set_action_button(bi->cancel,true);
	lv_obj_set_user_data(bi->cancel,bi);
	lv_obj_set_event_cb(bi->cancel,cancel_cb);
	lv_label_set_text(lv_label_create(bi->cancel,NULL),_("Cancel"));
	return 0;
}

struct gui_register guireg_bootitem={
	.name="bootitem",
	.title="Boot Item Editor",
	.icon="bootmgr.svg",
	.show_app=false,
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=bootitem_get_focus,
	.lost_focus=bootitem_lost_focus,
	.draw=draw_bootitem,
	.data_load=do_load,
	.resize=do_resize,
	.back=true,
	.mask=true,
};
#endif
