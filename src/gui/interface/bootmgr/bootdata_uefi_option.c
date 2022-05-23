/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"bootmgr.h"
#ifdef ENABLE_UEFI
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/UefiBootManagerLib.h>
#endif

static int bootdata_uefi_option_get_focus(struct gui_activity*d){
	struct bootdata_uefi_option*bi=d->data;
	if(!bi)return 0;
	lv_group_add_obj(gui_grp,bi->txt_opt);
	#ifdef ENABLE_UEFI
	lv_group_add_obj(gui_grp,bi->sel_opt);
	#endif
	lv_group_add_obj(gui_grp,bi->ok);
	lv_group_add_obj(gui_grp,bi->cancel);
	return 0;
}

static int bootdata_uefi_option_lost_focus(struct gui_activity*d){
	struct bootdata_uefi_option*bi=d->data;
	if(!bi)return 0;
	lv_group_remove_obj(bi->txt_opt);
	#ifdef ENABLE_UEFI
	lv_group_add_obj(gui_grp,bi->sel_opt);
	#endif
	lv_group_remove_obj(bi->ok);
	lv_group_remove_obj(bi->cancel);
	return 0;
}

static bool do_save(struct bootdata_uefi_option*bi){
	if(!bi->name[0])return false;
	char*end=NULL;
	const char*opt=lv_textarea_get_text(bi->txt_opt);
	if(!opt)return false;
	errno=0;
	uint option=(uint)strtol(opt,&end,16);
	if(errno!=0||end==opt||option>0xFFFF){
		msgbox_alert("Invalid number");
		return true;
	}
	confd_set_integer_dict(
		bootmgr_base,bi->name,
		"extra.option",
		(int64_t)option
	);
	bi->act->data_changed=true;
	return true;
}

static int init(struct gui_activity*act){
	if(!act->args)ERET(EINVAL);
	struct bootdata_uefi_option*bi=malloc(sizeof(struct bootdata_uefi_option));
	if(!bi)return -ENOMEM;
	memset(bi,0,sizeof(struct bootdata_uefi_option));
	act->data=bi,bi->act=act;
	return 0;
}

#ifdef ENABLE_UEFI
static void load_uefi_options(struct bootdata_uefi_option*bi,uint opt){
	uint pos=0;
	UINTN cnt=0;
	EFI_BOOT_MANAGER_LOAD_OPTION*bo;
	char name[BUFSIZ];
	ZeroMem(bi->options,sizeof(bi->options));
	lv_dropdown_clear_options(bi->sel_opt);
	lv_dropdown_add_option(bi->sel_opt,_("(none)"),0);
	EfiBootManagerRefreshAllBootOption();
	if((bo=EfiBootManagerGetLoadOptions(
		&cnt,LoadOptionTypeBoot
	))&&cnt>0)for(UINTN i=0;i<cnt;i++){
		ZeroMem(name,sizeof(name));
		AsciiSPrint(
			name,sizeof(name)-1,
			"%s (%04X)",
			bo[i].Description,
			bo[i].OptionNumber
		);
		pos++;
		bi->options[pos]=(uint)bo[i].OptionNumber;
		lv_dropdown_add_option(bi->sel_opt,name,pos);
		if(opt==bi->options[pos])
			lv_dropdown_set_selected(bi->sel_opt,pos);
		if(pos>=(uint)ARRLEN(bi->options)-1)break;
	}
	tlog_info("found %u options",(uint)pos);
}

static void option_cb(lv_obj_t*obj,lv_event_t e){
	char option[8];
	struct bootdata_uefi_option*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->sel_opt)return;
	lv_default_dropdown_cb(obj,e);
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	uint16_t o=lv_dropdown_get_selected(bi->sel_opt);
	memset(option,0,sizeof(option));
	if(o>0)snprintf(option,5,"%04X",bi->options[o]);
	lv_textarea_set_text(bi->txt_opt,option);
}
#endif

static void input_cb(lv_obj_t*obj,lv_event_t e){
	struct bootdata_uefi_option*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->txt_opt)return;
	lv_input_cb(obj,e);
	#ifdef ENABLE_UEFI
	if(e!=LV_EVENT_DEFOCUSED)return;
	char*end=NULL;
	const char*option=lv_textarea_get_text(bi->txt_opt);
	if(!option)return;
	errno=0;
	uint pos=0;
	uint opt=(uint)strtol(option,&end,16);
	if(errno!=0||end==option||opt>0xFFFF)return;
	for(uint i=0;i<ARRLEN(bi->options);i++){
		if(opt==bi->options[i]){
			pos=i;
			break;
		}
	}
	if(pos==0||pos>lv_dropdown_get_option_cnt(bi->sel_opt))return;
	lv_dropdown_set_selected(bi->sel_opt,pos);
	#endif
}

static int do_load(struct gui_activity*act){
	struct bootdata_uefi_option*bi=act->data;
	if(!bi)return 0;
	struct boot_config*c=NULL;
	if(act->args)c=boot_get_config(act->args),act->args=NULL;
	if(c){
		char option[8];
		uint opt=confd_get_integer_base(c->key,"option",0x10000);
		memset(option,0,sizeof(option));
		if(opt<=0xFFFF)snprintf(option,5,"%04X",opt);
		lv_textarea_set_text(bi->txt_opt,option);
		#ifdef ENABLE_UEFI
		load_uefi_options(bi,opt);
		#endif
		strncpy(bi->name,c->ident,sizeof(bi->name)-1);
	}
	if(c)free(c);
	if(!bi->name[0])ERET(EINVAL);
	return 0;
}

static int do_cleanup(struct gui_activity*act){
	struct bootdata_uefi_option*bi=act->data;
	if(!bi)return 0;
	memset(bi,0,sizeof(struct bootdata_uefi_option));
	free(bi);
	act->data=NULL;
	return 0;
}

static void ok_cb(lv_obj_t*obj,lv_event_t e){
	struct bootdata_uefi_option*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->ok||e!=LV_EVENT_CLICKED)return;
	if(do_save(bi))guiact_do_back();
}

static void cancel_cb(lv_obj_t*obj,lv_event_t e){
	struct bootdata_uefi_option*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->cancel||e!=LV_EVENT_CLICKED)return;
	guiact_do_back();
}

static int do_resize(struct gui_activity*act){
	lv_coord_t h=0,w=act->w/8*7,x=0;
	struct bootdata_uefi_option*bi=act->data;
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
	lv_obj_set_pos(bi->lbl_opt,x,h);
	lv_obj_align(
		bi->txt_opt,bi->lbl_opt,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/2,0
	);
	lv_obj_set_y(bi->txt_opt,h);
	lv_obj_align(
		bi->lbl_opt,bi->txt_opt,
		LV_ALIGN_OUT_LEFT_MID,
		-gui_font_size/2,0
	);
	lv_obj_set_width(
		bi->txt_opt,w-
		lv_obj_get_width(bi->lbl_opt)-
		gui_font_size
	);
	h+=lv_obj_get_height(bi->txt_opt);

	#ifdef ENABLE_UEFI
	h+=gui_font_size/2;
	lv_obj_set_width(bi->sel_opt,w);
	lv_obj_set_pos(bi->sel_opt,x,h);
	h+=lv_obj_get_height(bi->sel_opt);
	#endif

	h+=gui_font_size;
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

static int draw_bootdata_uefi_option(struct gui_activity*act){
	struct bootdata_uefi_option*bi=act->data;

	bi->page=lv_page_create(act->page,NULL);
	lv_obj_set_click(bi->page,false);

	bi->box=lv_obj_create(bi->page,NULL);
	lv_obj_set_click(bi->box,false);
	lv_obj_set_style_local_border_width(bi->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);

	// Title
	bi->title=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->title,_("Edit UEFI Option Boot Item"));
	lv_label_set_long_mode(bi->title,LV_LABEL_LONG_BREAK);

	bi->lbl_opt=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_opt,_("UEFI Option:"));

	bi->txt_opt=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_opt,"");
	lv_textarea_set_one_line(bi->txt_opt,true);
	lv_textarea_set_cursor_hidden(bi->txt_opt,true);
	lv_textarea_set_accepted_chars(bi->txt_opt,HEX);
	lv_obj_set_user_data(bi->txt_opt,bi);
	lv_obj_set_event_cb(bi->txt_opt,input_cb);

	#ifdef ENABLE_UEFI
	bi->sel_opt=lv_dropdown_create(bi->box,NULL);
	lv_obj_set_user_data(bi->sel_opt,bi);
	lv_obj_set_event_cb(bi->sel_opt,option_cb);
	#endif

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

struct gui_register guireg_bootdata_uefi_option={
	.name="bootdata-uefi-option",
	.title="Boot Item Extra Data Editor",
	.icon="bootmgr.svg",
	.show_app=false,
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=bootdata_uefi_option_get_focus,
	.lost_focus=bootdata_uefi_option_lost_focus,
	.draw=draw_bootdata_uefi_option,
	.data_load=do_load,
	.resize=do_resize,
	.back=true,
	.mask=true,
};
#endif
