/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"gui.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/activity.h"
#include"gui/filepicker.h"

lv_style_t*lv_style_opa_mask(){
	static lv_style_t bg;
	static bool initialized=false;
	if(!initialized){
		lv_style_init(&bg);
		lv_style_set_bg_opa(&bg,LV_OPA_50);
		lv_style_set_bg_color(&bg,lv_color_black());
		initialized=true;
	}
	return &bg;
}

void lv_style_set_btn_item(lv_obj_t*btn){
	lv_color_t dg=lv_color_make(192,192,192);
	lv_color_t lg=lv_color_make(64,64,64);
	lv_color_t bg=lv_obj_get_style_bg_color(lv_scr_act(),0);
	lv_color_t tc=lv_obj_get_style_text_color(lv_scr_act(),0);
	lv_color_t lb=gui_dark?lv_color_lighten(bg,LV_OPA_20):lv_color_darken(bg,LV_OPA_60);
	lv_obj_set_style_radius(btn,5,0);
	lv_obj_set_style_border_width(btn,1,0);
	lv_obj_set_style_border_color(btn,dg,0);
	lv_obj_set_style_border_color(btn,lg,LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(btn,bg,0);
	lv_obj_set_style_bg_color(btn,lb,LV_STATE_CHECKED);
	lv_obj_set_style_text_color(btn,tc,0);
	lv_obj_set_style_outline_width(btn,1,0);
	lv_obj_set_style_outline_color(btn,lg,0);
}

void lv_obj_set_small_text_font(lv_obj_t*obj,uint8_t part){
	lv_obj_set_style_text_font(obj,gui_font_small,part);
}

void lv_obj_set_enabled(lv_obj_t*obj,bool enabled){
	if(enabled)lv_obj_clear_state(obj,LV_STATE_DISABLED);
	else lv_obj_add_state(obj,LV_STATE_DISABLED);
}

void lv_obj_set_checked(lv_obj_t*obj,bool checked){
	if(checked)lv_obj_add_state(obj,LV_STATE_CHECKED);
	else lv_obj_clear_state(obj,LV_STATE_CHECKED);
}

bool lv_obj_is_checked(lv_obj_t*obj){
	return lv_obj_has_state(obj,LV_STATE_CHECKED);
}

void lv_textarea_remove_text(lv_obj_t*ta,uint32_t start,uint32_t len){
	if(len==0)return;
	lv_textarea_t*ext=(lv_textarea_t*)ta;
	char*txt=lv_label_get_text(ext->label);
	_lv_txt_cut(txt,start,len);
	lv_label_set_text(ext->label,txt);
	lv_textarea_clear_selection(ta);
	ext->sel_start=ext->sel_end=0;
	if(lv_obj_get_width(ext->label)==0){
		lv_coord_t bw=lv_obj_get_style_border_width(ta,LV_PART_CURSOR);
		lv_obj_set_width(ext->label,bw==0?1:bw);
	}
	lv_event_send(ta,LV_EVENT_VALUE_CHANGED,NULL);
}

void lv_obj_set_hidden(lv_obj_t*obj,bool hidden){
	if(hidden)lv_obj_add_flag(obj,LV_OBJ_FLAG_HIDDEN);
	else lv_obj_clear_flag(obj,LV_OBJ_FLAG_HIDDEN);
}

void lv_default_dropdown_cb(lv_event_t*e){
	if(e->code==LV_EVENT_DELETE)return;
	lv_group_set_editing(gui_grp,lv_dropdown_is_open(e->target));
}

void lv_img_fill_image(lv_obj_t*img,lv_coord_t w,lv_coord_t h){
	lv_img_t*x=(lv_img_t*)img;
	if(x->w<=0||x->h<=0)return;
	int b=(int)(((float)w/x->w)*256);
	int a=(int)(((float)h/x->h)*256);
	lv_img_set_zoom(img,MAX(a,b));
}

void lv_drag_border(lv_obj_t*obj,lv_coord_t width,lv_coord_t height,lv_coord_t border){
	lv_coord_t x=lv_obj_get_x(obj);
	lv_coord_t y=lv_obj_get_y(obj);
	lv_coord_t w=lv_obj_get_width(obj);
	lv_coord_t h=lv_obj_get_height(obj);
	if(x<border)x=border;
	if(y<border)y=border;
	if(x+w+border>width)x=width-w-border;
	if(y+h+border>height)y=height-h-border;
	lv_obj_set_pos(obj,x,y);
}

void lv_input_cb(lv_event_t*e){
	sysbar_focus_input(e->target);
	sysbar_keyboard_open();
}

static bool select_cb(bool ok,const char**path,uint16_t cnt,void*user_data){
	if(!ok)return false;
	if(!path||!path[0]||path[1]||cnt!=1)return true;
	lv_textarea_set_text(user_data,path[0]);
	lv_event_send(user_data,LV_EVENT_DEFOCUSED,NULL);
	return false;
}

static void sel_cb(lv_event_t*e){
	struct filepicker*fp=filepicker_create(select_cb,"Select item");
	filepicker_set_user_data(fp,e->user_data);
	filepicker_set_max_item(fp,1);
}

static void clr_cb(lv_event_t*e){
	lv_textarea_set_text(e->user_data,"");
}

lv_obj_t*lv_draw_side_clear_btn(lv_obj_t*box,lv_obj_t*txt,lv_coord_t size){
	lv_obj_t*clr=lv_draw_button(box,LV_SYMBOL_CLOSE,true,clr_cb,txt);
	lv_obj_set_style_radius(clr,LV_RADIUS_CIRCLE,0);
	lv_obj_set_style_size(clr,size,size);
	return clr;
}

lv_obj_t*lv_draw_button(
	lv_obj_t*parent,const char*title,
	bool enabled,lv_event_cb_t cb,void*user_data
){
	lv_obj_t*btn=lv_btn_create(parent);
	if(cb)lv_obj_add_event_cb(btn,cb,LV_EVENT_CLICKED,user_data);
	lv_obj_set_enabled(btn,enabled);
	lv_obj_t*lbl=lv_label_create(btn);
	lv_label_set_text(lbl,title);
	lv_obj_center(lbl);
	return btn;
}

static bool btn_dsc_va(struct button_dsc**dsc,size_t s,va_list va){
	if(!dsc)return false;
	size_t cnt=0;
	struct button_dsc*d;
	memset(dsc,0,sizeof(struct button_dsc*)*s);
	while((d=va_arg(va,struct button_dsc*))){
		if(cnt>=s){
			LV_LOG_WARN("too many buttons");
			return false;
		}
		dsc[cnt++]=d;
	}
	return true;
}

void lv_draw_buttons(
	lv_obj_t*parent,
	struct button_dsc**btns
){
	for(size_t i=0;btns[i];i++){
		lv_obj_t*btn=lv_draw_button_grid(
			parent,
			_(btns[i]->title),
			btns[i]->enabled,
			btns[i]->cb,
			btns[i]->user_data,
			btns[i]->col_pos,
			btns[i]->col_span,
			btns[i]->row_pos,
			btns[i]->row_span
		);
		if(btns[i]->style)lv_obj_add_style(
			btn,btns[i]->style,0
		);
		if(btns[i]->tgt)*btns[i]->tgt=btn;
	}
}

lv_obj_t*lv_draw_buttons_grid(
	lv_obj_t*parent,
	lv_coord_t*grid_col,
	lv_coord_t*grid_row,
	struct button_dsc**btns
){
	if(!parent||!grid_row||!grid_col)return NULL;
	lv_obj_t*box=lv_draw_line_wrapper(parent,grid_col,grid_row);
	lv_draw_buttons(box,btns);
	return box;
}

static void auto_free_cb(lv_event_t*e){
	lv_mem_free(e->user_data);
}

lv_obj_t*lv_draw_buttons_auto(
	lv_obj_t*parent,
	struct button_dsc**btns
){
	void*buf;
	size_t xs,ys;
	lv_coord_t x=0,y=0,*grid_col,*grid_row;
	if(!parent||!btns)return NULL;
	for(size_t i=0;btns[i];i++){
		x=lv_coord_max(x,btns[i]->col_pos+btns[i]->col_span);
		y=lv_coord_max(y,btns[i]->row_pos+btns[i]->row_span);
	}
	if(x<=0||y<=0)return NULL;
	xs=(x+2)*sizeof(lv_coord_t);
	ys=(y+2)*sizeof(lv_coord_t);
	if(!(buf=lv_mem_alloc(xs+ys)))return NULL;
	lv_memset_00(buf,xs+ys);
	grid_col=buf,grid_row=buf+xs;
	for(lv_coord_t i=0;i<x;i++)grid_col[i]=LV_GRID_FR(1);
	for(lv_coord_t i=0;i<y;i++)grid_row[i]=LV_SIZE_CONTENT;
	grid_col[x]=LV_GRID_TEMPLATE_LAST;
	grid_row[y]=LV_GRID_TEMPLATE_LAST;
	lv_obj_t*obj=lv_draw_buttons_grid(
		parent,grid_col,grid_row,btns
	);
	if(obj)lv_obj_add_event_cb(
		obj,auto_free_cb,
		LV_EVENT_DELETE,buf
	);
	return obj;
}

void lv_draw_buttons_va(lv_obj_t*parent,va_list va){
	struct button_dsc*btns[128];
	if(btn_dsc_va(btns,128,va))
		lv_draw_buttons(parent,btns);
}

void lv_draw_buttons_arg(lv_obj_t*parent,...){
	va_list va;
	va_start(va,parent);
	lv_draw_buttons_va(parent,va);
	va_end(va);
}

lv_obj_t*lv_draw_buttons_grid_va(
	lv_obj_t*parent,
	lv_coord_t*grid_col,
	lv_coord_t*grid_row,
	va_list va
){
	struct button_dsc*btns[128];
	return btn_dsc_va(btns,128,va)?
		lv_draw_buttons_grid(
			parent,grid_col,grid_row,btns
		):NULL;
}

lv_obj_t*lv_draw_buttons_grid_arg(
	lv_obj_t*parent,
	lv_coord_t*grid_col,
	lv_coord_t*grid_row,
	...
){
	va_list va;
	va_start(va,grid_row);
	lv_obj_t*r=lv_draw_buttons_grid_va(
		parent,grid_col,grid_row,va
	);
	va_end(va);
	return r;
}

lv_obj_t*lv_draw_buttons_auto_va(lv_obj_t*parent,va_list va){
	struct button_dsc*btns[128];
	return btn_dsc_va(btns,128,va)?
	       lv_draw_buttons_auto(parent,btns):NULL;
}

lv_obj_t*lv_draw_buttons_auto_arg(lv_obj_t*parent,...){
	va_list va;
	va_start(va,parent);
	lv_obj_t*r=lv_draw_buttons_auto_va(parent,va);
	va_end(va);
	return r;
}

lv_obj_t*lv_draw_button_grid(
	lv_obj_t*parent,
	const char*title,
	bool enabled,
	lv_event_cb_t cb,
	void*user_data,
	uint8_t col_pos,
	uint8_t col_span,
	uint8_t row_pos,
	uint8_t row_span
){
	lv_obj_t*btn=lv_draw_button(
		parent,title,enabled,
		cb,user_data
	);
	lv_obj_set_grid_cell(
		btn,
		LV_GRID_ALIGN_STRETCH,
		col_pos,col_span,
		LV_GRID_ALIGN_CENTER,
		row_pos,row_span
	);
	return btn;
}

lv_obj_t*lv_draw_checkbox(
	lv_obj_t*parent,
	const char*title,
	bool checked,
	lv_event_cb_t cb,
	void*user_data
){
	lv_obj_t*chk=lv_checkbox_create(parent);
	lv_checkbox_set_text(chk,_(title));
	lv_obj_set_checked(chk,checked);
	if(cb)lv_obj_add_event_cb(
		chk,cb,
		LV_EVENT_VALUE_CHANGED,
		user_data
	);
	return chk;
}

lv_obj_t*lv_draw_checkbox_grid(
	lv_obj_t*parent,
	const char*title,
	bool checked,
	lv_event_cb_t cb,
	void*user_data,
	lv_grid_align_t x_align,
	uint8_t col_pos,
	uint8_t col_span,
	lv_grid_align_t y_align,
	uint8_t row_pos,
	uint8_t row_span
){
	lv_obj_t*chk=lv_draw_checkbox(
		parent,title,checked,
		cb,user_data
	);
	lv_obj_set_grid_cell(
		chk,
		x_align,col_pos,col_span,
		y_align,row_pos,row_span
	);
	return chk;
}

lv_obj_t*lv_draw_dropdown(
	lv_obj_t*parent,
	char*title,
	lv_obj_t**dd
){
	static lv_coord_t grid_col[]={
		LV_GRID_CONTENT,
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	};
	lv_obj_t*box=lv_draw_line_wrapper(
		parent,grid_col,grid_row
	);
	lv_obj_t*lbl=lv_label_create(box);
	lv_label_set_text(lbl,_(title));
	lv_obj_set_grid_cell(
		lbl,
		LV_GRID_ALIGN_START,0,1,
		LV_GRID_ALIGN_CENTER,0,1
	);
	lv_obj_t*sel=lv_dropdown_create(box);
	lv_obj_add_event_cb(
		sel,lv_default_dropdown_cb,
		LV_EVENT_ALL,NULL
	);
	lv_obj_set_grid_cell(
		sel,
		LV_GRID_ALIGN_STRETCH,1,1,
		LV_GRID_ALIGN_STRETCH,0,1
	);
	if(dd)*dd=sel;
	return box;
}

lv_obj_t*lv_draw_input(
	lv_obj_t*parent,
	const char*title,
	lv_obj_t**chk,
	lv_obj_t**clr,
	lv_obj_t**txt,
	lv_obj_t**btn
){
	static lv_coord_t grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_col_r[]={
		LV_GRID_CONTENT,
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_row_r[]={
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	bool oneline=title&&!chk&&!clr&&!btn;
	lv_obj_t*box=lv_draw_line_wrapper(
		parent,
		oneline?grid_col_r:grid_col,
		oneline?grid_row_r:grid_row
	);

	lv_obj_t*t=lv_textarea_create(box);
	lv_textarea_set_text(t,"");
	lv_textarea_set_one_line(t,true);
	lv_obj_update_layout(t);
	lv_obj_set_grid_cell(
		t,
		LV_GRID_ALIGN_STRETCH,
		oneline?1:0,
		btn||oneline?1:2,
		LV_GRID_ALIGN_CENTER,
		title&&!oneline?1:0,
		title||oneline?1:2
	);
	if(txt)*txt=t;

	if(title){
		lv_obj_t*tit;
		if(chk){
			tit=lv_checkbox_create(box);
			lv_checkbox_set_text(tit,_(title));
			*chk=tit;
		}else{
			tit=lv_label_create(box);
			lv_label_set_text(tit,_(title));
		}
		lv_obj_set_grid_cell(
			tit,
			LV_GRID_ALIGN_START,
			0,clr||oneline?1:2,
			LV_GRID_ALIGN_CENTER,0,1
		);
		if(clr){
			*clr=lv_draw_side_clear_btn(
				box,t,gui_font_size
			);
			lv_obj_set_grid_cell(
				*clr,
				LV_GRID_ALIGN_END,1,1,
				LV_GRID_ALIGN_CENTER,0,1
			);
		}
	}

	if(btn)*btn=lv_draw_button_grid(
		box,"...",true,sel_cb,t,
		1,1,title?1:0,title?1:2
	);
	return box;
}

lv_obj_t*lv_draw_dialog_box(lv_obj_t*parent,lv_obj_t**lbl,const char*title){
	lv_obj_t*box=lv_obj_create(parent);
	lv_obj_set_flex_flow(box,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(box,gui_font_size/2,0);
	lv_obj_set_style_max_width(box,lv_pct(80),0);
	lv_obj_set_style_max_height(box,lv_pct(80),0);
	lv_obj_set_style_min_width(box,gui_dpi*2,0);
	lv_obj_set_style_min_height(box,gui_font_size*2,0);
	lv_obj_set_height(box,LV_SIZE_CONTENT);
	lv_obj_center(box);
	if(title){
		lv_obj_t*x=lv_label_create(box);
		lv_label_set_text(x,_(title));
		lv_label_set_long_mode(x,LV_LABEL_LONG_WRAP);
		lv_obj_set_style_text_align(x,LV_TEXT_ALIGN_CENTER,0);
		lv_obj_set_width(x,lv_pct(100));
		if(lbl)*lbl=x;
	}
	return box;
}

lv_obj_t*lv_draw_wrapper(
	lv_obj_t*parent,
	lv_coord_t*grid_col,
	lv_coord_t*grid_row,
	lv_coord_t width,
	lv_coord_t height
){
	lv_obj_t*wrapper=lv_obj_create(parent);
	lv_obj_set_style_radius(wrapper,0,0);
	lv_obj_set_scroll_dir(wrapper,LV_DIR_NONE);
	lv_obj_set_style_border_width(wrapper,0,0);
	lv_obj_set_style_bg_opa(wrapper,LV_OPA_0,0);
	lv_obj_set_style_pad_all(wrapper,gui_dpi/50,0);
	if(grid_col&&grid_row)lv_obj_set_grid_dsc_array(wrapper,grid_col,grid_row);
	lv_obj_clear_flag(wrapper,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_size(wrapper,width,height);
	return wrapper;
}

static void cancel_cb(lv_event_t*e __attribute__((unused))){
	sysbar_focus_input(NULL);
	sysbar_keyboard_close();
	guiact_do_back();
}

lv_obj_t*lv_draw_btns_ok_cancel(
	lv_obj_t*parent,
	lv_obj_t**btn_ok,
	lv_obj_t**btn_cancel,
	lv_event_cb_t ok_cb,
	void*user_data
){
	static lv_coord_t grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	lv_obj_t*btns=lv_draw_line_wrapper(parent,grid_col,grid_row);
	lv_draw_buttons_grid_arg(
		btns,grid_col,grid_row,
		#define BTN(tgt,title,cb,x)&(struct button_dsc){\
			tgt,true,_(title),cb,user_data,x,1,0,1,NULL\
		}
		BTN(btn_ok,     "OK",     ok_cb,    0),
		BTN(btn_cancel, "Cancel", cancel_cb,1),
		NULL
		#undef BTN
	);
	return btns;
}

static void lv_drag_event_handler(lv_event_t*e){
	lv_indev_t*i;
	lv_point_t v;
	lv_obj_t*o,*p;
	lv_coord_t m,x,y,w,h,rx,ry;
	if(!(o=lv_event_get_user_data(e)))
		o=lv_event_get_target(e);
	i=lv_indev_get_act();
	if(!i)return;
	p=lv_obj_get_parent(o);
	lv_indev_get_vect(i,&v);
	if(v.x==0&&v.y==0)return;
	m=gui_font_size/2;
	x=lv_obj_get_x(o)+v.x,y=lv_obj_get_y(o)+v.y;
	w=lv_obj_get_width(p)-lv_obj_get_width(o)-m;
	h=lv_obj_get_height(p)-lv_obj_get_height(o)-m;
	rx=lv_coord_border(x,w,m),ry=lv_coord_border(y,h,m);
	lv_obj_set_pos(o,rx,ry);
	i->proc.types.pointer.scroll_obj=o;
	lv_event_stop_bubbling(e);
}

static void lv_drag_released_handler(lv_event_t*e __attribute__((unused))){
	lv_indev_t*i=lv_indev_get_act();
	if(i)i->proc.types.pointer.scroll_obj=NULL;
}

static void lv_obj_add_drag_child(lv_obj_t*root,lv_obj_t*p){
	lv_obj_add_event_cb(
		p,lv_drag_event_handler,
		LV_EVENT_PRESSING,root
	);
	lv_obj_add_event_cb(
		p,lv_drag_released_handler,
		LV_EVENT_RELEASED,NULL
	);
	for(uint32_t i=0;i<lv_obj_get_child_cnt(p);i++)
		lv_obj_add_drag_child(root,lv_obj_get_child(p,i));
}

void lv_obj_add_drag(lv_obj_t*o){
	lv_obj_add_drag_child(o,o);
}

void lv_event_dump_handler(lv_event_t*e){
	log_debug(
		"event","target %p code %s",
		lv_event_get_target(e),
		lv_event_string[lv_event_get_code(e)]
	);
}

void lv_img_src_try(
	lv_obj_t*img,
	const char*prefix,
	const char*id,
	const char*path
){
	char icon[64];
	lv_img_t*x=(lv_img_t*)img;
	if(!img||!prefix||!prefix[0])return;
	if(path&&path[0]){
		lv_img_set_src(img,path);
		if(x->w>0&&x->h>0)return;
	}
	if(id&&id[0]){
		memset(icon,0,sizeof(icon));
		snprintf(
			icon,sizeof(icon)-1,
			"@%s-%s",prefix,id
		);
		lv_img_set_src(img,icon);
		if(x->w>0&&x->h>0)return;
	}
	memset(icon,0,sizeof(icon));
	snprintf(
		icon,sizeof(icon)-1,
		"@%s-unknown",prefix
	);
	lv_img_set_src(img,icon);
}
#endif
