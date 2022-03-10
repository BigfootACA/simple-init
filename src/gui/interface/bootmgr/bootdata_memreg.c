/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"bootmgr.h"

static int bootdata_memory_region_get_focus(struct gui_activity*d){
	struct bootdata_memory_region*bi=d->data;
	if(!bi)return 0;
	lv_group_add_obj(gui_grp,bi->txt_reg_base);
	lv_group_add_obj(gui_grp,bi->txt_reg_size);
	lv_group_add_obj(gui_grp,bi->txt_reg_end);
	lv_group_add_obj(gui_grp,bi->ok);
	lv_group_add_obj(gui_grp,bi->cancel);
	(void)bootmgr_base;
	return 0;
}

static int bootdata_memory_region_lost_focus(struct gui_activity*d){
	struct bootdata_memory_region*bi=d->data;
	if(!bi)return 0;
	lv_group_remove_obj(bi->txt_reg_base);
	lv_group_remove_obj(bi->txt_reg_size);
	lv_group_remove_obj(bi->txt_reg_end);
	lv_group_remove_obj(bi->ok);
	lv_group_remove_obj(bi->cancel);
	return 0;
}

static bool do_save(struct bootdata_memory_region*bi){
	char*es=NULL;
	if(!bi->path[0])return false;
	const char*base=lv_textarea_get_text(bi->txt_reg_base);
	const char*size=lv_textarea_get_text(bi->txt_reg_size);
	if(!base||!size)return false;
	errno=0;
	long long ab=strtoll(base,&es,16);
	if(errno!=0||es==base||ab<=0){
		msgbox_alert("Invalid memory address or size");
		return false;
	}
	long long as=strtoll(size,&es,16);
	if(errno!=0||es==size||as<=0){
		msgbox_alert("Invalid memory address or size");
		return false;
	}
	confd_delete_base(bi->path,"start");
	confd_delete_base(bi->path,"base");
	confd_delete_base(bi->path,"size");
	confd_delete_base(bi->path,"end");
	confd_set_integer_base(bi->path,"base",ab);
	confd_set_integer_base(bi->path,"size",as);
	bi->act->data_changed=true;
	return true;
}

static int init(struct gui_activity*act){
	if(!act->args)ERET(EINVAL);
	struct bootdata_memory_region*bi=malloc(sizeof(struct bootdata_memory_region));
	if(!bi)return -ENOMEM;
	memset(bi,0,sizeof(struct bootdata_memory_region));
	act->data=bi,bi->act=act;
	return 0;
}

static int do_load(struct gui_activity*act){
	char buff[64];
	struct bootdata_memory_region*bi=act->data;
	if(!bi||!act->args)return 0;
	strncpy(bi->path,act->args,sizeof(bi->path)-1);
	int64_t base=confd_get_integer_base(bi->path,"base",-1);
	int64_t start=confd_get_integer_base(bi->path,"start",-1);
	int64_t size=confd_get_integer_base(bi->path,"size",-1);
	int64_t end=confd_get_integer_base(bi->path,"end",-1);
	if(start>0){
		if(base>0&&start!=base)tlog_warn("base and start mismatched");
		base=start;
	}
	if(end>0){
		if(size>0&&end!=base+size)tlog_warn("size and end mismatched");
		size=end-base;
	}
	if(base<0)base=0;
	if(size<0)size=0;
	memset(buff,0,sizeof(buff));
	snprintf(buff,sizeof(buff)-1,"0x%llX",(unsigned long long)base);
	lv_textarea_set_text(bi->txt_reg_base,buff);
	memset(buff,0,sizeof(buff));
	snprintf(buff,sizeof(buff)-1,"0x%llX",(unsigned long long)size);
	lv_textarea_set_text(bi->txt_reg_size,buff);
	memset(buff,0,sizeof(buff));
	snprintf(buff,sizeof(buff)-1,"0x%llX",(unsigned long long)base+size);
	lv_textarea_set_text(bi->txt_reg_end,buff);
	act->args=NULL;
	return 0;
}

static int do_cleanup(struct gui_activity*act){
	struct bootdata_memory_region*bi=act->data;
	if(!bi)return 0;
	memset(bi,0,sizeof(struct bootdata_memory_region));
	free(bi);
	act->data=NULL;
	return 0;
}

static void input_cb(lv_obj_t*obj,lv_event_t e){
	struct gui_activity*act;
	long long sb=0,ss=0,se=0;
	struct bootdata_memory_region*bi;
	char*es=NULL,nb[64],ns[64],ne[64];
	if(!(act=lv_obj_get_user_data(obj)))return;
	if(!(bi=act->data))return;
	lv_input_cb(obj,e);
	if(e!=LV_EVENT_DEFOCUSED)return;
	const char*base=lv_textarea_get_text(bi->txt_reg_base);
	const char*size=lv_textarea_get_text(bi->txt_reg_size);
	const char*end=lv_textarea_get_text(bi->txt_reg_end);
	if(!base||!size||!end)return;
	errno=0;
	long long ab=strtoll(base,&es,16);
	if(errno!=0||es==base||ab<=0)return;
	long long as=strtoll(size,&es,16);
	if(errno!=0||es==size||as<=0)return;
	long long ae=strtoll(end,&es,16);
	if(errno!=0||es==end||ae<=0)return;
	memset(nb,0,sizeof(nb));
	memset(ns,0,sizeof(ns));
	memset(ne,0,sizeof(ne));
	if(obj==bi->txt_reg_end)ss=MAX(0,ae-ab);
	else if(obj==bi->txt_reg_size)se=ab+as;
	else if(obj==bi->txt_reg_base)se=ab+as;
	if(se!=0)snprintf(ne,sizeof(ne)-1,"0x%llX",se);
	if(ss!=0)snprintf(ns,sizeof(ns)-1,"0x%llX",ss);
	if(sb!=0)snprintf(nb,sizeof(nb)-1,"0x%llX",sb);
	if(ne[0])lv_textarea_set_text(bi->txt_reg_end,ne);
	if(ns[0])lv_textarea_set_text(bi->txt_reg_size,ns);
	if(nb[0])lv_textarea_set_text(bi->txt_reg_base,nb);
}

static void ok_cb(lv_obj_t*obj,lv_event_t e){
	struct bootdata_memory_region*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->ok||e!=LV_EVENT_CLICKED)return;
	if(do_save(bi))guiact_do_back();
}

static void cancel_cb(lv_obj_t*obj,lv_event_t e){
	struct bootdata_memory_region*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->cancel||e!=LV_EVENT_CLICKED)return;
	guiact_do_back();
}

static int do_resize(struct gui_activity*act){
	lv_coord_t h=0,w=act->w/8*7,x=0;
	struct bootdata_memory_region*bi=act->data;
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
	lv_obj_set_pos(bi->lbl_reg_base,x,h);
	h+=lv_obj_get_height(bi->lbl_reg_base);

	lv_obj_set_pos(bi->txt_reg_base,x,h);
	lv_obj_set_width(bi->txt_reg_base,w);
	h+=lv_obj_get_height(bi->txt_reg_base);

	h+=gui_font_size/2;
	lv_obj_set_pos(bi->lbl_reg_size,x,h);
	h+=lv_obj_get_height(bi->lbl_reg_size);

	lv_obj_set_pos(bi->txt_reg_size,x,h);
	lv_obj_set_width(bi->txt_reg_size,w);
	h+=lv_obj_get_height(bi->txt_reg_size);

	h+=gui_font_size/2;
	lv_obj_set_pos(bi->lbl_reg_end,x,h);
	h+=lv_obj_get_height(bi->lbl_reg_end);

	lv_obj_set_pos(bi->txt_reg_end,x,h);
	lv_obj_set_width(bi->txt_reg_end,w);
	h+=lv_obj_get_height(bi->txt_reg_end);

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

static int draw_bootdata_memory_region(struct gui_activity*act){
	struct bootdata_memory_region*bi=act->data;

	bi->page=lv_page_create(act->page,NULL);
	lv_obj_set_click(bi->page,false);

	bi->box=lv_obj_create(bi->page,NULL);
	lv_obj_set_click(bi->box,false);
	lv_obj_set_style_local_border_width(bi->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);

	// Title
	bi->title=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->title,_("Edit Memory Region"));
	lv_label_set_long_mode(bi->title,LV_LABEL_LONG_BREAK);

	bi->lbl_reg_base=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_reg_base,_("Memory Start Address (Hex):"));

	bi->txt_reg_base=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_reg_base,"");
	lv_textarea_set_one_line(bi->txt_reg_base,true);
	lv_textarea_set_cursor_hidden(bi->txt_reg_base,true);
	lv_textarea_set_accepted_chars(bi->txt_reg_base,HEX"x");
	lv_obj_set_user_data(bi->txt_reg_base,act);
	lv_obj_set_event_cb(bi->txt_reg_base,input_cb);

	bi->lbl_reg_size=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_reg_size,_("Memory Size (Hex):"));

	bi->txt_reg_size=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_reg_size,"");
	lv_textarea_set_one_line(bi->txt_reg_size,true);
	lv_textarea_set_cursor_hidden(bi->txt_reg_size,true);
	lv_textarea_set_accepted_chars(bi->txt_reg_size,HEX"x");
	lv_obj_set_user_data(bi->txt_reg_size,act);
	lv_obj_set_event_cb(bi->txt_reg_size,input_cb);

	bi->lbl_reg_end=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_reg_end,_("Memory End Address (hex):"));

	bi->txt_reg_end=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_reg_end,"");
	lv_textarea_set_one_line(bi->txt_reg_end,true);
	lv_textarea_set_cursor_hidden(bi->txt_reg_end,true);
	lv_textarea_set_accepted_chars(bi->txt_reg_end,HEX"x");
	lv_obj_set_user_data(bi->txt_reg_end,act);
	lv_obj_set_event_cb(bi->txt_reg_end,input_cb);

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

struct gui_register guireg_bootdata_memreg={
	.name="bootdata-memory-region",
	.title="Boot Item Extra Data Editor",
	.icon="bootmgr.svg",
	.show_app=false,
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=bootdata_memory_region_get_focus,
	.lost_focus=bootdata_memory_region_lost_focus,
	.draw=draw_bootdata_memory_region,
	.data_load=do_load,
	.resize=do_resize,
	.back=true,
	.mask=true,
};
#endif
