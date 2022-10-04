/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_HIVEX
#define _GNU_SOURCE
#include<hivex.h>
#include<stdlib.h>
#include<string.h>
#include"gui.h"
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"regedit.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"gui/inputbox.h"
#include"gui/filepicker.h"
#define TAG "regedit"

struct reg_item{
	struct regedit*reg;
	bool parent;
	char name[255];
	hive_node_h node;
	hive_value_h dir;
	hive_value_h value;
	hive_type type;
	lv_obj_t*btn,*lbl,*w_img,*img,*val,*xtype;
};

static char*get_string_path(struct regedit*reg,char*sub){
	if(!reg)return NULL;
	static char string[BUFSIZ],*p,*s,*e;
	if(!reg->path)return sub?sub:strcpy(string,"\\");
	memset(string,0,sizeof(string));
	p=string,s=p,e=p+BUFSIZ-1;
	*p++='\\';
	list*o=list_first(reg->path);
	if(o)do{
		if(p!=s&&*(p-1)!='\\')*p++='\\';
		LIST_DATA_DECLARE(item,o,char*);
		strncpy(p,item,e-p-1);
		p+=strlen(item);
	}while((o=o->next));
	if(sub){
		if(p!=s&&*(p-1)!='\\')*p++='\\';
		strncpy(p,sub,e-p-1);
	}
	return string;
}

static const char*get_icon(struct reg_item*ci){
	if(!ci)return NULL;
	if(ci->parent)return "inode-parent";
	switch(ci->type){
		case hive_t_REG_NONE:return "inode-dir";
		default:return "text-x-plain";
	}
	return "unknown";
}

static int clean_item(void*d){
	struct reg_item*i=(struct reg_item*)d;
	if(i){
		if(i->btn)lv_obj_del(i->btn);
		free(i);
	}
	return 0;
}

static void clean_view(struct regedit*reg){
	if(!reg)return;
	if(reg->info)lv_obj_del(reg->info);
	list_free_all(reg->items,clean_item);
	reg->items=NULL,reg->info=NULL,reg->last_btn=NULL;
	lv_obj_set_enabled(reg->btn_delete,false);
	lv_obj_set_enabled(reg->btn_edit,false);
}

static void set_info(struct regedit*reg,char*text){
	if(!reg||!text)return;
	clean_view(reg);
	if(reg->info)lv_obj_del(reg->info);
	reg->info=lv_label_create(reg->view);
	lv_label_set_long_mode(reg->info,LV_LABEL_LONG_WRAP);
	lv_obj_set_width(reg->info,lv_pct(100));
	lv_obj_set_style_text_align(reg->info,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(reg->info,text);
}

static void load_view(struct regedit*reg);
static void go_back(struct regedit*reg){
	if(!reg||!reg->path)return;
	hive_node_h par=hivex_node_parent(reg->hive,reg->node);
	if(par<=0)return;
	reg->node=par;
	list_obj_del(&reg->path,list_last(reg->path),list_default_free);
	load_view(reg);
}

static void click_item(struct reg_item*ci){
	if(!ci||!ci->reg)return;
	if(ci->type==hive_t_REG_NONE){
		if(ci->parent)go_back(ci->reg);
		else if(ci->node>0){
			list_obj_add_new_strdup(&ci->reg->path,ci->name);
			ci->reg->node=ci->node;
			load_view(ci->reg);
		}
	}else{
		static struct regedit_value value;
		value.hive=ci->reg->hive;
		value.value=ci->value;
		value.node=ci->dir;
		value.reg=ci->reg;
		guiact_start_activity(
			&guireg_regedit_value,&value
		);
	}
}

static size_t get_selected(struct regedit*reg){
	size_t c=0;
	list*o=list_first(reg->items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct reg_item*);
		if(item&&item->btn&&lv_obj_is_checked(item->btn))c++;
	}while((o=o->next));
	return c;
}

static void check_item(struct reg_item*ci,bool checked){
	if(!ci||!ci->reg)return;
	if(ci->parent){
		go_back(ci->reg);
		return;
	}
	lv_obj_set_checked(ci->btn,checked);
	size_t c=get_selected(ci->reg);
	if(c==0){
		lv_obj_set_enabled(ci->reg->btn_delete,false);
		lv_obj_set_enabled(ci->reg->btn_edit,false);
	}else if(c==1){
		lv_obj_set_enabled(ci->reg->btn_delete,true);
		lv_obj_set_enabled(ci->reg->btn_edit,true);
	}else{
		lv_obj_set_enabled(ci->reg->btn_delete,true);
		lv_obj_set_enabled(ci->reg->btn_edit,false);
	}
}

static void item_click(lv_event_t*e){
	struct reg_item*ci=e->user_data;
	lv_indev_t*i=lv_indev_get_act();
	if(i&&i->proc.long_pr_sent)return;
	size_t c=get_selected(ci->reg);
	if(c>0)check_item(ci,!lv_obj_is_checked(ci->btn));
	else click_item(ci);
}

static void item_check(lv_event_t*e){
	struct reg_item*ci=e->user_data;
	if(ci->parent)return;
	e->stop_processing=1;
	check_item(ci,!lv_obj_is_checked(ci->btn));
}

static void add_item(struct reg_item*ci){
	char buf[256]={0};
	static lv_coord_t grid_col[]={
		0,
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	if(!ci||!ci->reg)return;
	if(grid_col[0]==0)grid_col[0]=gui_font_size*3;

	// reg item button
	ci->btn=lv_btn_create(ci->reg->view);
	lv_obj_set_width(ci->btn,lv_pct(100));
	lv_obj_set_content_height(ci->btn,grid_col[0]);
	lv_style_set_btn_item(ci->btn);
	lv_obj_set_grid_dsc_array(ci->btn,grid_col,grid_row);
	lv_obj_add_event_cb(ci->btn,item_click,LV_EVENT_CLICKED,ci);
	lv_obj_add_event_cb(ci->btn,item_check,LV_EVENT_LONG_PRESSED,ci);
	lv_group_add_obj(gui_grp,ci->btn);
	if(list_obj_add_new(&ci->reg->items,ci)!=0){
		telog_error("cannot add reg item list");
		abort();
	}

	// reg image
	ci->w_img=lv_obj_create(ci->btn);
	lv_obj_set_size(ci->w_img,grid_col[0],grid_col[0]);
	lv_obj_clear_flag(ci->w_img,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(ci->w_img,LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_border_width(ci->w_img,0,0);
	lv_obj_set_style_bg_opa(ci->w_img,LV_OPA_0,0);
	lv_obj_set_grid_cell(
		ci->w_img,
		LV_GRID_ALIGN_STRETCH,0,1,
		LV_GRID_ALIGN_STRETCH,0,2
	);
	ci->img=lv_img_create(ci->w_img);
	lv_img_src_try(ci->img,"mime",get_icon(ci),NULL);
	lv_img_set_size_mode(ci->img,LV_IMG_SIZE_MODE_REAL);
	lv_img_fill_image(ci->img,grid_col[0],grid_col[0]);
	lv_obj_center(ci->img);

	// reg name and checkbox
	ci->lbl=lv_label_create(ci->btn);
	lv_label_set_text(ci->lbl,ci->parent?_("Parent key"):ci->name);
	if(ci->type!=hive_t_REG_NONE){
		lv_obj_set_grid_cell(
			ci->lbl,
			LV_GRID_ALIGN_START,1,1,
			LV_GRID_ALIGN_CENTER,0,1
		);
		ci->val=lv_label_create(ci->btn);
		lv_label_set_long_mode(ci->val,LV_LABEL_LONG_DOT);
		lv_obj_set_small_text_font(ci->val,LV_PART_MAIN);
		lv_obj_set_grid_cell(
			ci->val,
			LV_GRID_ALIGN_STRETCH,1,2,
			LV_GRID_ALIGN_CENTER,1,1
		);
		hivex_value_to_string(buf,256,ci->reg->hive,ci->value);
		lv_label_set_text(ci->val,buf);
		ci->xtype=lv_label_create(ci->btn);
		lv_obj_set_style_text_align(ci->xtype,LV_TEXT_ALIGN_RIGHT,0);
		lv_label_set_long_mode(ci->xtype,LV_LABEL_LONG_CLIP);
		lv_label_set_text(ci->xtype,hivex_type_to_string(ci->type));
		lv_obj_set_grid_cell(
			ci->xtype,
			LV_GRID_ALIGN_STRETCH,2,1,
			LV_GRID_ALIGN_CENTER,0,1
		);
	}else lv_obj_set_grid_cell(
		ci->lbl,
		LV_GRID_ALIGN_START,1,2,
		LV_GRID_ALIGN_CENTER,0,2
	);
}

static void add_node_item(struct regedit*reg,bool parent,hive_node_h dir,hive_node_h n){
	if(!reg)return;
	char*key;
	struct reg_item*ci=malloc(sizeof(struct reg_item));
	if(!ci)return;
	memset(ci,0,sizeof(struct reg_item));
	if(!parent&&(key=hivex_node_name(reg->hive,n))){
		strncpy(ci->name,key,sizeof(ci->name)-1);
		free(key);
	}
	ci->parent=parent;
	ci->node=n;
	ci->reg=reg;
	ci->dir=dir;
	add_item(ci);
}

static void add_value_item(struct regedit*reg,hive_node_h dir,hive_value_h v){
	if(!reg)return;
	char*key=hivex_value_key(reg->hive,v);
	struct reg_item*ci=malloc(sizeof(struct reg_item));
	if(!ci||!key)goto fail;
	memset(ci,0,sizeof(struct reg_item));
	size_t len;
	if(hivex_value_type(reg->hive,v,&ci->type,&len)==-1)goto fail;
	strncpy(ci->name,key,sizeof(ci->name)-1);
	free(key);
	ci->parent=false;
	ci->value=v;
	ci->reg=reg;
	ci->dir=dir;
	add_item(ci);
	return;
	fail:
	if(ci)free(ci);
	if(key)free(key);
}

static void load_view(struct regedit*reg){
	if(!reg)return;
	size_t i=0;
	lv_label_set_text(reg->lbl_path,get_string_path(reg,NULL));
	clean_view(reg);
	if(reg->hive&&reg->node){
		hive_node_h pa=hivex_node_parent(reg->hive,reg->node);
		if(pa&&reg->path)add_node_item(reg,true,0,pa);
		hive_node_h*cs=hivex_node_children(reg->hive,reg->node);
		if(cs){
			for(i=0;cs[i];i++)add_node_item(reg,false,reg->node,cs[i]);
			free(cs);
		}
		hive_value_h*vs=hivex_node_values(reg->hive,reg->node);
		if(vs){
			for(i=0;vs[i];i++)add_value_item(reg,reg->node,vs[i]);
			free(vs);
		}
	}else set_info(reg,_("nothing here"));
}

static int regedit_get_focus(struct gui_activity*d){
	struct regedit*reg=d->data;
	if(!reg)return 0;
	load_view(reg);
	lv_group_add_obj(gui_grp,reg->btn_add);
	lv_group_add_obj(gui_grp,reg->btn_reload);
	lv_group_add_obj(gui_grp,reg->btn_delete);
	lv_group_add_obj(gui_grp,reg->btn_edit);
	lv_group_add_obj(gui_grp,reg->btn_home);
	lv_group_add_obj(gui_grp,reg->btn_save);
	lv_group_add_obj(gui_grp,reg->btn_load);
	lv_obj_set_enabled(reg->btn_save,reg->changed);
	return 0;
}

static int regedit_lost_focus(struct gui_activity*d){
	struct regedit*reg=d->data;
	if(!reg)return 0;
	list*o=list_first(reg->items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct reg_item*);
		if(item->btn)lv_group_remove_obj(item->btn);
	}while((o=o->next));
	lv_group_remove_obj(reg->btn_add);
	lv_group_remove_obj(reg->btn_reload);
	lv_group_remove_obj(reg->btn_delete);
	lv_group_remove_obj(reg->btn_edit);
	lv_group_remove_obj(reg->btn_home);
	lv_group_remove_obj(reg->btn_save);
	lv_group_remove_obj(reg->btn_load);
	return 0;
}

static void close_file(struct regedit*reg){
	lv_obj_set_enabled(reg->btn_save,false);
	lv_obj_set_enabled(reg->btn_reload,false);
	lv_obj_set_enabled(reg->btn_delete,false);
	lv_obj_set_enabled(reg->btn_edit,false);
	lv_obj_set_enabled(reg->btn_add,false);
	lv_obj_set_enabled(reg->btn_home,false);
	if(reg->hive)hivex_close(reg->hive);
	list_free_all_def(reg->path);
	reg->node=0,reg->root=0,reg->changed=false;
	reg->hive=NULL,reg->path=NULL;
	load_view(reg);
}

static void load_file(struct regedit*reg,char*xpath){
	if(!reg)return;
	close_file(reg);
	reg->hive=hivex_open(xpath,HIVEX_OPEN_WRITE);
	if(!reg->hive){
		msgbox_alert("open registry file '%s' failed: %m",xpath);
		return;
	}
	if((reg->root=hivex_root(reg->hive))<=0){
		msgbox_alert("open registry root failed: %m");
		close_file(reg);
		return;
	}
	tlog_debug("loaded file %s",xpath);
	reg->node=reg->root,reg->changed=false;
	lv_obj_set_enabled(reg->btn_save,false);
	lv_obj_set_enabled(reg->btn_reload,true);
	lv_obj_set_enabled(reg->btn_add,true);
	lv_obj_set_enabled(reg->btn_home,true);
	load_view(reg);
}

static bool reg_load_cb(bool ok,const char**p,uint16_t cnt,void*user_data){
	if(!ok)return false;
	if(!p||!p[0]||p[1]||cnt!=1)return true;
	load_file((struct regedit*)user_data,(char*)p[0]);
	return false;
}

struct create_data{
	uint16_t id;
	struct regedit*reg;
};

static bool reg_create_input_cb(bool ok,const char*content,void*user_data){
	if(!ok)return false;
	if(!content||!content[0])return true;
	bool b;
	struct hive_set_value sv;
	struct create_data*data=user_data;
	sv.len=1,sv.key=(char*)content;
	sv.value=(char[]){0,0,0,0,0,0,0,0};
	switch(data->id){
		case 0:{
			b=hivex_node_add_child(
				data->reg->hive,
				data->reg->node,
				content
			)==0;
			if(b)msgbox_alert("Add registry key failed: %m");
			return b;
		}break;
		case 1:sv.t=hive_t_REG_SZ;break;
		case 2:sv.t=hive_t_REG_MULTI_SZ;break;
		case 3:sv.t=hive_t_REG_DWORD,sv.len=sizeof(int32_t);break;
		case 4:sv.t=hive_t_REG_QWORD,sv.len=sizeof(int64_t);break;
		case 5:sv.t=hive_t_REG_BINARY;break;
		default:return true;
	}
	b=hivex_node_set_value(
		data->reg->hive,
		data->reg->node,
		&sv,0
	)!=0;
	if(b)msgbox_alert("Add registry value failed: %m");
	data->reg->changed=true;
	return b;
}

static bool reg_create_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	if(id>5)return false;
	static struct create_data data;
	data.id=id,data.reg=user_data;
	inputbox_set_user_data(inputbox_create(
		reg_create_input_cb,
		"Input new name"
	),&data);
	return false;
}

static bool reg_save_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	if(id!=0)return false;
	struct regedit*reg=user_data;
	if(hivex_commit(reg->hive,NULL,0)==0){
		reg->changed=false;
		lv_obj_set_enabled(reg->btn_save,reg->changed);
		tlog_debug("file saved");
	}else{
		msgbox_alert("Save registry file failed: %m");
		tlog_warn("save registry failed: %m");
	}
	return false;
}

static bool reg_delete_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	list*l;
	if(id!=0)return false;
	struct regedit*reg=user_data;
	bool failed=false,change_value=false;
	size_t len=hivex_node_nr_values(reg->hive,reg->node);
	hive_value_h*vs=hivex_node_values(reg->hive,reg->node);
	if(!vs){
		telog_warn("get values failed");
		failed=true;
	}else if((l=list_first(reg->items)))do{
		LIST_DATA_DECLARE(item,l,struct reg_item*);
		if(!lv_obj_is_checked(item->btn))continue;
		if(
			item->node&&!item->value&&item->type==hive_t_REG_NONE&&
			hivex_node_delete_child(reg->hive,item->node)!=0
		){
			telog_warn(
				"delete child key %zu(%s) failed",
				item->node,item->name
			);
			failed=true;
		}
		if(!item->node&&item->value&&item->type!=hive_t_REG_NONE){
			bool found=false;
			for(size_t i=0;i<len;i++){
				if(vs[i]!=item->value)continue;
				vs[i]=0,found=true;
			}
			if(found)change_value=true;
			else{
				telog_warn(
					"search child value %zu(%s) failed",
					item->node,item->name
				);
				failed=true;
			}
		}
		reg->changed=true;
	}while((l=l->next));
	if(change_value&&!failed){
		size_t new_len=0;
		struct hive_set_value*sv;
		size_t size=(len+1)*sizeof(struct hive_set_value);
		if(!(sv=malloc(size))){
			telog_warn("failed to allocate set values");
			failed=true;
		}else{
			memset(sv,0,size);
			for(size_t i=0;i<len;i++){
				if(!vs[i])continue;
				if(!(sv[new_len].key=hivex_value_key(
					reg->hive,vs[i]
				))||!(sv[new_len].value=hivex_value_value(
					reg->hive,vs[i],
					&sv[new_len].t,
					&sv[new_len].len
				))){
					telog_warn("failed to get value %zu",vs[i]);
					failed=true;
					break;
				}
				new_len++;
			}
			if(!failed&&hivex_node_set_values(
				reg->hive,reg->node,new_len,sv,0
			)!=0){
				telog_warn("failed to set values");
				failed=true;
			}
			for(size_t i=0;i<len;i++){
				if(sv[i].value)free(sv[i].value);
				if(sv[i].key)free(sv[i].key);
			}
			free(sv);
		}
	}
	if(failed)msgbox_alert("One or more items failed to delete");
	return false;
}

static void btns_cb(lv_event_t*e){
	list*l;
	static struct regedit_value value;
	static const char*create_buttons[]={
		"Registry Key",
		"String Value",
		"Multi String Value",
		"DWORD Value (int32)",
		"QWORD Value (int64)",
		"Binary Data Value",
		"Cancel",
		""
	};
	struct regedit*reg=e->user_data;
	if(!reg||strcmp(guiact_get_last()->name,"regedit")!=0)return;
	value.hive=reg->hive,value.reg=reg;
	value.value=0,value.node=0;
	if(e->target==reg->btn_add){
		msgbox_set_user_data(msgbox_create_custom(
			reg_create_cb,
			create_buttons,
			"Select type to add"
		),reg);
	}else if(e->target==reg->btn_reload){
		load_view(reg);
	}else if(e->target==reg->btn_delete){
		msgbox_set_user_data(msgbox_create_yesno(
			reg_delete_cb,
			"Are you sure you want to delete selected items?"
		),reg);
	}else if(e->target==reg->btn_edit){
		if((l=list_first(reg->items)))do{
			LIST_DATA_DECLARE(item,l,struct reg_item*);
			if(lv_obj_is_checked(item->btn)){
				value.value=item->value;
				value.node=item->dir;
			}
		}while((l=l->next));
		if(value.value)guiact_start_activity(
			&guireg_regedit_value,&value
		);
	}else if(e->target==reg->btn_home){
		list_free_all_def(reg->path);
		reg->path=NULL,reg->node=reg->root;
		load_view(reg);
	}else if(e->target==reg->btn_save){
		if(reg->changed)msgbox_set_user_data(msgbox_create_yesno(
			reg_save_cb,
			"Notice: Registry Editor is currently in an early beta version, "
			"it MAY DAMAGE YOUR REGISTRY, are you sure you want to continue save?"
		),reg);
	}else if(e->target==reg->btn_load){
		struct filepicker*fp=filepicker_create(reg_load_cb,"Select registry file to load");
		filepicker_set_max_item(fp,1);
		filepicker_set_user_data(fp,reg);
	}
}

static int regedit_draw(struct gui_activity*act){
	struct regedit*reg=(struct regedit*)act->data;
	if(!reg)return 0;
	reg->scr=act->page;

	lv_obj_set_flex_flow(reg->scr,LV_FLEX_FLOW_COLUMN);

	reg->view=lv_obj_create(reg->scr);
	lv_obj_set_width(reg->view,lv_pct(100));
	lv_obj_set_style_border_width(reg->view,0,0);
	lv_obj_set_flex_flow(reg->view,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_grow(reg->view,1);

	// current path
	reg->lbl_path=lv_label_create(reg->scr);
	lv_obj_set_width(reg->lbl_path,lv_pct(100));
	lv_obj_set_style_text_align(reg->lbl_path,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_long_mode(
		reg->lbl_path,
		confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SCROLL_CIRCULAR:
		LV_LABEL_LONG_DOT
	);
	lv_label_set_text(reg->lbl_path,get_string_path(reg,NULL));

	lv_draw_buttons_auto_arg(
		act->page,
		#define BTN(tgt,en,title,x)&(struct button_dsc){\
			&tgt,en,_(title),btns_cb,reg,x,1,0,1,NULL\
		}
		BTN(reg->btn_add,    false, LV_SYMBOL_PLUS,    0),
		BTN(reg->btn_reload, false, LV_SYMBOL_REFRESH, 1),
		BTN(reg->btn_delete, false, LV_SYMBOL_TRASH,   2),
		BTN(reg->btn_edit,   false, LV_SYMBOL_EDIT,    3),
		BTN(reg->btn_home,   false, LV_SYMBOL_HOME,    4),
		BTN(reg->btn_save,   false, LV_SYMBOL_SAVE,    5),
		BTN(reg->btn_load,   true,  LV_SYMBOL_UPLOAD,  6),
		#undef BTN
		NULL
	);

	if(act->args)load_file(reg,act->args);
	msgbox_alert(
		"Notice: Registry Editor is currently in an early beta version, "
		"it MAY DAMAGE YOUR REGISTRY, please backup and use it with caution."
	);
	return 0;
}

static int do_clean(struct gui_activity*d){
	struct regedit*reg=d->data;
	if(!reg)return 0;
	list_free_all_def(reg->items);
	list_free_all_def(reg->path);
	if(reg->hive)hivex_close(reg->hive);
	reg->items=NULL,reg->path=NULL,reg->info=NULL;
	reg->hive=NULL,reg->root=0,reg->node=0;
	free(reg);
	d->data=NULL;
	return 0;
}

static bool back_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	struct regedit*reg=user_data;
	if(id==0){
		reg->changed=false;
		guiact_do_back();
		guiact_do_back();
		return true;
	}
	return false;
}

static int do_back(struct gui_activity*d){
	struct regedit*reg=d->data;
	if(!reg)return 0;
	if(reg->path){
		go_back(reg);
		return 1;
	}
	if(!reg->changed)return 0;
	msgbox_set_user_data(msgbox_create_yesno(
		back_cb,
		"Registry has changed, "
		"do you want to discard any changes?"
	),reg);
	return 1;
}

static int do_init(struct gui_activity*d){
	struct regedit*reg=malloc(sizeof(struct regedit));
	if(!reg)ERET(ENOMEM);
	memset(reg,0,sizeof(struct regedit));
	d->data=reg;
	return 0;
}

struct gui_register guireg_regedit={
	.name="regedit",
	.title="Registry Editor",
	.show_app=true,
	.open_file=true,
	.open_regex=(char*[]){
		".*/windows/system32/config/.+",
		".*/software$",
		".*/system$",
		".*/security$",
		".*/sam$",
		".*/software$",
		".*/bcd$",
		NULL
	},
	.init=do_init,
	.ask_exit=do_back,
	.quiet_exit=do_clean,
	.get_focus=regedit_get_focus,
	.lost_focus=regedit_lost_focus,
	.draw=regedit_draw,
	.back=true
};
#endif
#endif
