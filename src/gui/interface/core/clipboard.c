/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<time.h>
#include"gui.h"
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/inputbox.h"
#include"gui/activity.h"
#include"gui/clipboard.h"
#define TAG "clipboard"
#define BASE "gui.clipboard"
#define BASE_HIST BASE".history"

static enum clipboard_type clip_type=CLIP_NULL;
static char*clip_cont=NULL;
struct clipboard_item;
struct clipboard_app{
	lv_obj_t*list,*info;
	lv_obj_t*add,*reload,*clear;
	lv_obj_t*use,*delete,*edit;
	list*items;
	struct clipboard_item*selected;
};
struct clipboard_item{
	list*item;
	lv_obj_t*btn,*chk;
	char key[256];
	struct clipboard_app*clip;
};

void clipboard_reset(void){
	confd_delete_base(BASE,"last");
	if(clip_cont)free(clip_cont);
	clip_type=CLIP_NULL,clip_cont=NULL;
	tlog_debug("reset clipboard");
}

void clipboard_clear(void){
	clipboard_reset();
	confd_delete(BASE);
}

void clipboard_load(const char*key){
	if(!key)return;
	clip_type=confd_get_integer_dict(BASE_HIST,key,"type",CLIP_NULL);
	clip_cont=confd_get_string_dict(BASE_HIST,key,"content",NULL);
	if(!clip_cont)clip_type=CLIP_NULL;
}

void clipboard_init(void){
	char*last=confd_get_string_base(BASE,"last",NULL);
	if(!last)return;
	clipboard_load(last);
	if(clip_type!=CLIP_NULL)tlog_debug(
		"restore clipboard type %d, content %s",
		clip_type,clip_cont
	);
	free(last);
}

int clipboard_set(enum clipboard_type type,const char*content,size_t len){
	if(type==CLIP_NULL){
		if(content)return -EINVAL;
		clipboard_reset();
		return 0;
	}
	if(!content)return -EINVAL;
	char*cont=len==0?strdup(content):strndup(content,len);
	if(!cont)return -ENOMEM;
	if(clip_cont){
		if(
			clip_type==type&&
			strcmp(clip_cont,cont)==0
		)return 0;
		free(clip_cont);
	}
	int c=1;
	char buf[255];
	time_t t;
	clip_type=type,clip_cont=cont;
	tlog_debug("set clipboard to type %d, content %s",type,cont);
	time(&t);
	for(;;){
		memset(buf,0,sizeof(buf));
		snprintf(buf,sizeof(buf)-1,"%ld%04d",(long int)t,c);
		if(confd_get_type_base(BASE_HIST,buf)!=TYPE_KEY)break;
		if(++c>=10000)return -EAGAIN;
	}
	confd_set_integer_dict(BASE_HIST,buf,"type",type);
	confd_set_string_dict(BASE_HIST,buf,"content",cont);
	confd_set_string_base(BASE,"last",buf);
	return 0;
}

enum clipboard_type clipboard_get_type(void){
	return clip_type;
}

char*clipboard_get_content(void){
	return clip_cont;
}

static void clipboard_item_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct clipboard_item*item=lv_obj_get_user_data(obj);
	if(!item||!item->clip)return;
	bool found=false;
	list*f=list_first(item->clip->items);
	if(f)do{
		LIST_DATA_DECLARE(i,f,struct clipboard_item*);
		if(i==item){
			found=true;
			continue;
		}
		lv_checkbox_set_checked(i->chk,false);
		lv_obj_set_checked(i->btn,false);
	}while((f=f->next));
	if(!found)return;
	lv_checkbox_set_checked(item->chk,true);
	lv_obj_set_checked(item->btn,true);
	lv_style_set_action_button(item->clip->use,true);
	lv_style_set_action_button(item->clip->edit,true);
	lv_style_set_action_button(item->clip->delete,true);
	item->clip->selected=item;
}

static void clipboard_add_item(struct clipboard_app*clip,lv_obj_t**last,char*key){
	char*str=confd_get_string_dict(BASE_HIST,key,"content",NULL);
	if(!str)return;
	struct clipboard_item*i=malloc(sizeof(struct clipboard_item));
	if(!i){
		free(str);
		return;
	}
	memset(i,0,sizeof(struct clipboard_item));
	strncpy(i->key,key,sizeof(i->key)-1);
	i->clip=clip;
	lv_coord_t bw;
	bw=lv_page_get_scrl_width(i->clip->list);

	i->btn=lv_btn_create(i->clip->list,NULL);
	lv_obj_set_size(i->btn,bw,gui_dpi/3);
	lv_obj_align(
		i->btn,*last,
		*last?LV_ALIGN_OUT_BOTTOM_MID:LV_ALIGN_IN_TOP_MID,
		0,*last?gui_dpi/8:0
	);
	lv_style_set_btn_item(i->btn);
	lv_obj_set_click(i->btn,false);
	*last=i->btn;

	i->chk=lv_checkbox_create(i->btn,NULL);
	lv_checkbox_ext_t*e=lv_obj_get_ext_attr(i->chk);
	lv_label_set_long_mode(e->label,confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SROLL_CIRC:
		LV_LABEL_LONG_DOT
	);
	lv_obj_set_width(e->label,bw-gui_dpi/5*2);
	lv_checkbox_set_text(i->chk,str);
	lv_obj_set_user_data(i->chk,i);
	lv_obj_set_event_cb(i->chk,clipboard_item_click);
	lv_style_set_focus_checkbox(i->chk);
	lv_group_add_obj(gui_grp,i->chk);

	i->item=list_new_notnull(i);
	list_obj_add(&clip->items,i->item);
	free(str);
}

static void clipboard_reload(struct clipboard_app*clip){
	if(clip->info)lv_obj_del(clip->info);
	list_free_all_def(clip->items);
	clip->items=NULL,clip->selected=NULL;
	lv_list_clean(clip->list);
	lv_style_set_action_button(clip->use,false);
	lv_style_set_action_button(clip->edit,false);
	lv_style_set_action_button(clip->delete,false);
	lv_obj_t*last=NULL;
	bool found=false;
	char**cs=confd_ls(BASE_HIST);
	if(cs){
		for(size_t s=0;cs[s];s++){
			found=true;
			clipboard_add_item(clip,&last,cs[s]);
		}
		if(cs[0])free(cs[0]);
		free(cs);
	}
	if(!found){
		clip->info=lv_label_create(clip->list,NULL);
		lv_label_set_long_mode(clip->info,LV_LABEL_LONG_BREAK);
		lv_obj_set_size(clip->info,lv_page_get_scrl_width(clip->list),gui_sh/16);
		lv_label_set_align(clip->info,LV_LABEL_ALIGN_CENTER);
		lv_label_set_text(clip->info,_("(none)"));
	}
}

static bool input_cb(bool ok,const char*content,void*user_data __attribute__((unused))){
	if(ok)clipboard_set(CLIP_TEXT,content,0);
	return false;
}

static void clipboard_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct clipboard_app*clip=lv_obj_get_user_data(obj);
	if(!clip)return;
	struct clipboard_item*sel=clip->selected;
	if(obj==clip->add){
		struct inputbox*in=inputbox_create(input_cb,"Add into clipboard");
		inputbox_set_one_line(in,false);
	}else if(obj==clip->reload){
		clipboard_reload(clip);
	}else if(obj==clip->clear){
		clipboard_clear();
		clipboard_reload(clip);
	}else if(obj==clip->use){
		if(!sel)return;
		confd_set_string_base(BASE,"last",sel->key);
		clipboard_load(sel->key);
	}else if(obj==clip->delete){
		if(!sel)return;
		char*last=confd_get_string_base(BASE,"last",NULL);
		if(last){
			if(strcmp(sel->key,last)==0){
				list*l=sel->item;
				struct clipboard_item*la=NULL;
				if(l->prev)la=LIST_DATA(l->prev,struct clipboard_item*);
				if(l->next)la=LIST_DATA(l->next,struct clipboard_item*);
				if(la){
					confd_set_string_base(BASE,"last",la->key);
					clipboard_load(la->key);
				}
			}
			free(last);
		}
		confd_delete_base(BASE_HIST,sel->key);
		clipboard_reload(clip);
	}else if(obj==clip->edit){
		if(!sel)return;
		struct inputbox*in=inputbox_create(input_cb,"Edit clipboard item");
		inputbox_set_one_line(in,false);
		char*str=confd_get_string_dict(BASE_HIST,sel->key,"content",NULL);
		if(str){
			inputbox_set_content(in,"%s",str);
			free(str);
		}
	}
}

static int clipboard_get_focus(struct gui_activity*act){
	struct clipboard_app*clip=act->data;
	if(!clip)return 0;
	clipboard_reload(clip);
	lv_group_add_obj(gui_grp,clip->add);
	lv_group_add_obj(gui_grp,clip->reload);
	lv_group_add_obj(gui_grp,clip->clear);
	lv_group_add_obj(gui_grp,clip->use);
	lv_group_add_obj(gui_grp,clip->delete);
	lv_group_add_obj(gui_grp,clip->edit);
	return 0;
}

static int clipboard_lost_focus(struct gui_activity*act){
	struct clipboard_app*clip=act->data;
	if(!clip)return 0;
	lv_group_remove_obj(clip->add);
	lv_group_remove_obj(clip->reload);
	lv_group_remove_obj(clip->clear);
	lv_group_remove_obj(clip->use);
	lv_group_remove_obj(clip->delete);
	lv_group_remove_obj(clip->edit);
	return 0;
}

static int clipboard_exit(struct gui_activity*act){
	free(act->data);
	act->data=NULL;
	return 0;
}

static int clipboard_draw(struct gui_activity*act){
	struct clipboard_app*clip=malloc(sizeof(struct clipboard_app));
	if(!clip)return -ENOMEM;
	memset(clip,0,sizeof(struct clipboard_app));
	act->data=clip;

	lv_coord_t btm=gui_dpi/10;
	lv_coord_t btw=gui_sw/3-btm;
	lv_coord_t bth=gui_font_size+gui_dpi/10;

	// function title
	lv_obj_t*title=lv_label_create(act->page,NULL);
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_y(title,gui_font_size);
	lv_obj_set_size(title,gui_sw,gui_font_size*2);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(title,_("Clipboard"));

	// options list
	clip->list=lv_page_create(act->page,NULL);
	lv_obj_set_pos(clip->list,gui_dpi/20,gui_font_size*3);
	lv_obj_set_size(clip->list,gui_sw-gui_dpi/10,gui_sh-lv_obj_get_y(clip->list)-bth*2-btm*3);
	lv_obj_set_style_local_border_width(clip->list,LV_LIST_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(clip->list,LV_LIST_PART_BG,LV_STATE_FOCUSED,0);
	lv_obj_set_style_local_border_width(clip->list,LV_LIST_PART_BG,LV_STATE_PRESSED,0);

	// button style
	static lv_style_t btn_style;
	lv_style_init(&btn_style);
	lv_style_set_radius(&btn_style,LV_STATE_DEFAULT,2);
	lv_style_set_outline_width(&btn_style,LV_STATE_PRESSED,0);

	// add button
	clip->add=lv_btn_create(act->page,NULL);
	lv_obj_set_size(clip->add,btw,bth);
	lv_obj_align(clip->add,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-(bth+btm*2));
	lv_obj_set_user_data(clip->add,clip);
	lv_obj_set_event_cb(clip->add,clipboard_click);
	lv_style_set_action_button(clip->add,true);
	lv_label_set_text(lv_label_create(clip->add,NULL),_("Add"));

	// reload button
	clip->reload=lv_btn_create(act->page,NULL);
	lv_obj_set_size(clip->reload,btw,bth);
	lv_obj_align(clip->reload,NULL,LV_ALIGN_IN_BOTTOM_MID,0,-(bth+btm*2));
	lv_obj_set_user_data(clip->reload,clip);
	lv_obj_set_event_cb(clip->reload,clipboard_click);
	lv_style_set_action_button(clip->reload,true);
	lv_label_set_text(lv_label_create(clip->reload,NULL),_("Reload"));

	// clear button
	clip->clear=lv_btn_create(act->page,NULL);
	lv_obj_set_size(clip->clear,btw,bth);
	lv_obj_align(clip->clear,NULL,LV_ALIGN_IN_BOTTOM_RIGHT,-btm,-(bth+btm*2));
	lv_obj_set_user_data(clip->clear,clip);
	lv_obj_set_event_cb(clip->clear,clipboard_click);
	lv_style_set_action_button(clip->clear,true);
	lv_label_set_text(lv_label_create(clip->clear,NULL),_("Clear"));

	// use button
	clip->use=lv_btn_create(act->page,NULL);
	lv_obj_set_size(clip->use,btw,bth);
	lv_obj_align(clip->use,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-btm);
	lv_obj_set_user_data(clip->use,clip);
	lv_obj_set_event_cb(clip->use,clipboard_click);
	lv_style_set_action_button(clip->use,false);
	lv_label_set_text(lv_label_create(clip->use,NULL),_("Use"));

	// delete button
	clip->delete=lv_btn_create(act->page,NULL);
	lv_obj_set_size(clip->delete,btw,bth);
	lv_obj_align(clip->delete,NULL,LV_ALIGN_IN_BOTTOM_MID,0,-btm);
	lv_obj_set_user_data(clip->delete,clip);
	lv_obj_set_event_cb(clip->delete,clipboard_click);
	lv_style_set_action_button(clip->delete,false);
	lv_label_set_text(lv_label_create(clip->delete,NULL),_("Delete"));

	// edit button
	clip->edit=lv_btn_create(act->page,NULL);
	lv_obj_set_size(clip->edit,btw,bth);
	lv_obj_align(clip->edit,NULL,LV_ALIGN_IN_BOTTOM_RIGHT,-btm,-btm);
	lv_obj_set_user_data(clip->edit,clip);
	lv_obj_set_event_cb(clip->edit,clipboard_click);
	lv_style_set_action_button(clip->edit,false);
	lv_label_set_text(lv_label_create(clip->edit,NULL),_("Edit"));

	clipboard_reload(clip);
	return 0;
}

struct gui_register guireg_clipboard={
	.name="clipboard",
	.title="Clipboard",
	.icon="clipboard.png",
	.show_app=true,
	.quiet_exit=clipboard_exit,
	.get_focus=clipboard_get_focus,
	.lost_focus=clipboard_lost_focus,
	.draw=clipboard_draw,
	.back=true,
};

#endif
