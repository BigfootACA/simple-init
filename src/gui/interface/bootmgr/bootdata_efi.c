/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"bootmgr.h"

static int bootdata_efi_get_focus(struct gui_activity*d){
	struct bootdata_efi*bi=d->data;
	if(!bi)return 0;
	lv_group_add_obj(gui_grp,bi->txt_path);
	lv_group_add_obj(gui_grp,bi->txt_guid);
	lv_group_add_obj(gui_grp,bi->txt_options);
	lv_group_add_obj(gui_grp,bi->chk_char16);
	lv_group_add_obj(gui_grp,bi->ok);
	lv_group_add_obj(gui_grp,bi->cancel);
	return 0;
}

static int bootdata_efi_lost_focus(struct gui_activity*d){
	struct bootdata_efi*bi=d->data;
	if(!bi)return 0;
	lv_group_remove_obj(bi->txt_path);
	lv_group_remove_obj(bi->txt_guid);
	lv_group_remove_obj(bi->txt_options);
	lv_group_remove_obj(bi->chk_char16);
	lv_group_remove_obj(bi->ok);
	lv_group_remove_obj(bi->cancel);
	return 0;
}

static bool do_save(struct bootdata_efi*bi){
	if(!bi->name[0])return false;
	const char*efi=lv_textarea_get_text(bi->txt_path);
	const char*guid=lv_textarea_get_text(bi->txt_guid);
	const char*opt=lv_textarea_get_text(bi->txt_options);
	bool w_char=lv_checkbox_is_checked(bi->chk_char16);
	if(!efi||!guid||!opt)return false;
	if(efi[0]&&guid[0]){
		msgbox_alert("Conflict options");
		return false;
	}
	if(!efi[0])confd_delete_dict(bootmgr_base,bi->name,"extra.efi_path");
	else confd_set_string_dict(bootmgr_base,bi->name,"extra.efi_path",(char*)efi);
	if(!guid[0])confd_delete_dict(bootmgr_base,bi->name,"extra.efi_fv_guid");
	else confd_set_string_dict(bootmgr_base,bi->name,"extra.efi_fv_guid",(char*)guid);
	if(!opt[0]){
		confd_delete_dict(bootmgr_base,bi->name,"extra.options");
		confd_delete_dict(bootmgr_base,bi->name,"extra.options_widechar");
	}else{
		confd_set_string_dict(bootmgr_base,bi->name,"extra.options",(char*)opt);
		confd_set_boolean_dict(bootmgr_base,bi->name,"extra.options_widechar",w_char);
	}
	bi->act->data_changed=true;
	return true;
}

static int init(struct gui_activity*act){
	if(!act->args)ERET(EINVAL);
	struct bootdata_efi*bi=malloc(sizeof(struct bootdata_efi));
	if(!bi)return -ENOMEM;
	memset(bi,0,sizeof(struct bootdata_efi));
	act->data=bi,bi->act=act;
	return 0;
}

static int do_load(struct gui_activity*act){
	struct bootdata_efi*bi=act->data;
	if(!bi)return 0;
	struct boot_config*c=NULL;
	if(act->args)c=boot_get_config(act->args),act->args=NULL;
	if(c){
		char*efi=confd_get_string_base(c->key,"efi_file",NULL);
		char*guid=confd_get_string_base(c->key,"efi_fv_guid",NULL);
		char*opt=confd_get_string_base(c->key,"options",NULL);
		bool char16=confd_get_boolean_base(c->key,"options_widechar",false);
		lv_textarea_set_text(bi->txt_path,efi?efi:"");
		lv_textarea_set_text(bi->txt_guid,guid?guid:"");
		lv_textarea_set_text(bi->txt_options,opt?opt:"");
		lv_checkbox_set_checked(bi->chk_char16,char16);
		strncpy(bi->name,c->ident,sizeof(bi->name)-1);
	}
	if(c)free(c);
	if(!bi->name[0])ERET(EINVAL);
	return 0;
}

static int do_cleanup(struct gui_activity*act){
	struct bootdata_efi*bi=act->data;
	if(!bi)return 0;
	memset(bi,0,sizeof(struct bootdata_efi));
	free(bi);
	act->data=NULL;
	return 0;
}

static void ok_cb(lv_obj_t*obj,lv_event_t e){
	struct bootdata_efi*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->ok||e!=LV_EVENT_CLICKED)return;
	if(do_save(bi))guiact_do_back();
}

static void cancel_cb(lv_obj_t*obj,lv_event_t e){
	struct bootdata_efi*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->cancel||e!=LV_EVENT_CLICKED)return;
	guiact_do_back();
}

static int do_resize(struct gui_activity*act){
	lv_coord_t h=0,w=act->w/8*7,x=0;
	struct bootdata_efi*bi=act->data;
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
	lv_obj_set_pos(bi->lbl_path,x,h);
	h+=lv_obj_get_height(bi->lbl_path);

	lv_obj_set_pos(bi->txt_path,x,h);
	lv_obj_set_width(bi->txt_path,w);
	h+=lv_obj_get_height(bi->txt_path);

	h+=gui_font_size/2;
	lv_obj_set_pos(bi->lbl_guid,x,h);
	h+=lv_obj_get_height(bi->lbl_guid);

	lv_obj_set_pos(bi->txt_guid,x,h);
	lv_obj_set_width(bi->txt_guid,w);
	h+=lv_obj_get_height(bi->txt_guid);

	h+=gui_font_size/2;
	lv_obj_set_pos(bi->lbl_options,x,h);
	h+=lv_obj_get_height(bi->lbl_options);

	lv_obj_set_pos(bi->txt_options,x,h);
	lv_obj_set_width(bi->txt_options,w);
	h+=lv_obj_get_height(bi->txt_options);

	h+=gui_font_size/2;
	lv_obj_set_pos(bi->chk_char16,x,h);
	h+=lv_obj_get_height(bi->chk_char16);

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

static int draw_bootdata_efi(struct gui_activity*act){
	struct bootdata_efi*bi=act->data;

	bi->page=lv_page_create(act->page,NULL);
	lv_obj_set_click(bi->page,false);

	bi->box=lv_obj_create(bi->page,NULL);
	lv_obj_set_click(bi->box,false);
	lv_obj_set_style_local_border_width(bi->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);

	// Title
	bi->title=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->title,_("Edit EFI Boot Item"));
	lv_label_set_long_mode(bi->title,LV_LABEL_LONG_BREAK);

	bi->lbl_path=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_path,_("EFI File Path:"));

	bi->txt_path=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_path,"");
	lv_textarea_set_one_line(bi->txt_path,true);
	lv_textarea_set_cursor_hidden(bi->txt_path,true);
	lv_obj_set_user_data(bi->txt_path,bi);
	lv_obj_set_event_cb(bi->txt_path,lv_input_cb);

	bi->lbl_guid=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_guid,_("EFI Firmware Volume Guid:"));

	bi->txt_guid=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_guid,"");
	lv_textarea_set_one_line(bi->txt_guid,true);
	lv_textarea_set_cursor_hidden(bi->txt_guid,true);
	lv_textarea_set_accepted_chars(bi->txt_guid,HEX"{-}");
	lv_obj_set_user_data(bi->txt_guid,bi);
	lv_obj_set_event_cb(bi->txt_guid,lv_input_cb);

	bi->lbl_options=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_options,_("EFI Load Options:"));

	bi->txt_options=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_options,"");
	lv_textarea_set_one_line(bi->txt_options,true);
	lv_textarea_set_cursor_hidden(bi->txt_options,true);
	lv_obj_set_user_data(bi->txt_options,bi);
	lv_obj_set_event_cb(bi->txt_options,lv_input_cb);

	bi->chk_char16=lv_checkbox_create(bi->box,NULL);
	lv_obj_set_user_data(bi->chk_char16,bi);
	lv_checkbox_set_text(bi->chk_char16,_("Use options as wide char"));
	lv_checkbox_set_checked(bi->chk_char16,true);
	lv_style_set_focus_checkbox(bi->chk_char16);

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

struct gui_register guireg_bootdata_efi={
	.name="bootdata-efi",
	.title="Boot Item Extra Data Editor",
	.icon="bootmgr.svg",
	.show_app=false,
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=bootdata_efi_get_focus,
	.lost_focus=bootdata_efi_lost_focus,
	.draw=draw_bootdata_efi,
	.data_load=do_load,
	.resize=do_resize,
	.back=true,
	.mask=true,
};
#endif
