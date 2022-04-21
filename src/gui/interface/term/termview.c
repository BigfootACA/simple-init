/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_LIBTSM
#include<stdio.h>
#include<stdarg.h>
#include"gui/termview.h"
#include"shl-llog.h"
#include"libtsm.h"
#define LV_OBJX_NAME "lv_termview"

void lv_termview_resize(lv_obj_t*tv){
	if(!tv)return;
	uint32_t cols,rows;
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	if(!term||!term->screen||!term->vte)return;
	term->width=lv_obj_get_width(tv);
	term->height=lv_obj_get_height(tv);
	size_t size=LV_CANVAS_BUF_SIZE_TRUE_COLOR(
		term->width,
		term->height
	);
	if(size!=term->mem_size){
		if(term->buffer){
			lv_img_cache_invalidate_src(
				lv_canvas_get_img(tv)
			);
			lv_mem_free(term->buffer);
		}
		if(!(term->buffer=lv_mem_alloc(size))){
			LV_LOG_ERROR("cannot allocate buffer for canvas");
			return;
		}
		term->mem_size=size;
		term->canvas.dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
		term->canvas.dsc.header.w  = term->width;
		term->canvas.dsc.header.h  = term->height;
		term->canvas.dsc.data      = (void*)term->buffer;
		lv_img_set_src(tv,&term->canvas.dsc);
	}
	if(!term->cust_font){
		term->font_reg=(lv_font_t*)lv_obj_get_style_text_font(
			tv,LV_TERMVIEW_PART_BG
		);
		term->font_bold=term->font_reg;
		term->font_ital=term->font_reg;
		term->font_bold_ital=term->font_reg;
	}
	if(!term->font_reg)return;
	if(term->glyph_height!=term->font_reg->line_height){
		term->glyph_height=term->font_reg->line_height;
		term->glyph_width=term->font_reg->line_height/2;
	}
	if(term->glyph_height<=0||term->glyph_width<=0){
		LV_LOG_WARN("invalid glyph size");
		return;
	}
	cols=term->width/term->glyph_width;
	rows=term->height/term->glyph_height;
	if(cols!=term->cols||rows!=term->rows){
		term->cols=cols,term->rows=rows;
		tsm_screen_resize(term->screen,cols,rows);
		if(term->resize_cb)
			term->resize_cb(tv,cols,rows);
	}
	lv_color_t bg=lv_obj_get_style_bg_color(
		tv,
		LV_TERMVIEW_PART_BG
	);
	lv_canvas_fill_bg(tv,bg,LV_OPA_COVER);
	term->max_sb=LV_MATH_MAX(
		term->max_sb,
		(uint32_t)(cols*rows*128)
	);
	tsm_screen_set_max_sb(
		term->screen,
		term->max_sb
	);
	term->age=0;
	lv_termview_update(tv);
}

static lv_res_t lv_termview_signal(
	lv_obj_t*tv,
	lv_signal_t sign,
	void*param
){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	lv_res_t res=term->sig(tv,sign,param);
	if(res!=LV_RES_OK)return res;
	switch(sign){
		case LV_SIGNAL_GET_TYPE:
			return lv_obj_handle_get_type_signal(
				param,LV_OBJX_NAME
			);
		case LV_SIGNAL_CLEANUP:{
			lv_img_cache_invalidate_src(lv_canvas_get_img(tv));
			if(term->screen)tsm_screen_unref(term->screen);
			if(term->vte)tsm_vte_unref(term->vte);
			if(term->buffer)lv_mem_free(term->buffer);
		}break;
		case LV_SIGNAL_PRESSING:{
			lv_point_t p;
			lv_indev_t*act=lv_indev_get_act();
			lv_indev_get_point(act,&p);
			if(term->drag_y_last==0)term->drag_y_last=p.y;
			else{
				bool up=true;
				lv_coord_t y=p.y-term->drag_y_last;
				if(y<0)y=-y,up=false;
				int ln=y/term->font_reg->line_height;
				if(ln>0){
					term->drag_y_last=p.y;
					if(up)tsm_screen_sb_up(term->screen,ln);
					else tsm_screen_sb_down(term->screen,ln);
					lv_termview_update(tv);
				}
			}
		};break;
		case LV_SIGNAL_RELEASED:
			term->drag_y_last=0;
		break;
		case LV_SIGNAL_STYLE_CHG:
		case LV_SIGNAL_COORD_CHG:
			lv_termview_resize(tv);
		break;
	}
	return res;
}

static int term_draw_cell(
	struct tsm_screen*screen,
	uint64_t id __attribute__((unused)),
	const uint32_t*cs,
	size_t len,
	unsigned int cw,
	unsigned int px,
	unsigned int py,
	const struct tsm_screen_attr*a,
	tsm_age_t age,
	void *data
){
	char xs[8];
	lv_obj_t*tv=data;
	lv_color_t fc,bc;
	lv_coord_t dw,dh,x,y;
	lv_draw_rect_dsc_t rd;
	lv_draw_label_dsc_t ld;
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	if(!term||screen!=term->screen||cw<=0)return 0;
	if(px>=term->cols||py>=term->rows)return 0;
	if(age&&term->age&&age<=term->age)return 0;
	lv_draw_rect_dsc_init(&rd);
	fc=LV_COLOR_MAKE(a->fr,a->fg,a->fb);
	bc=LV_COLOR_MAKE(a->br,a->bg,a->bb);
	rd.bg_color=a->inverse?fc:bc;
	x=px*term->glyph_width,y=py*term->glyph_height;
	dw=term->glyph_width*cw,dh=term->glyph_height;
	if(px==term->cols-1||dh+x>term->width)dw=term->width-x;
	if(py==term->rows-1||dh+y>term->height)dh=term->height-y;
	lv_canvas_draw_rect(tv,x,y,dw,dh,&rd);
	if(len<=0)return 0;
	lv_draw_label_dsc_init(&ld);
	ld.color=a->inverse?bc:fc;
	if(a->bold&&a->italic)ld.font=term->font_bold_ital;
	else if(a->italic)ld.font=term->font_ital;
	else if(a->bold)ld.font=term->font_bold;
	else ld.font=term->font_reg;
	if(!ld.font)return 0;
	if(a->underline)ld.decor=LV_TEXT_DECOR_UNDERLINE;
	for(size_t i=0;i<len;i++){
		memset(xs,0,sizeof(xs));
		tsm_ucs4_to_utf8(cs[i],xs);
		dw=term->glyph_width*cw;
		if(x+dw>term->width)dw=term->width-x;
		lv_canvas_draw_text(
			tv,x,y,dw,&ld,xs,
			LV_LABEL_ALIGN_LEFT
		);
		x+=dw;
	}
	return 0;
}

static void log_cb(
	void*data __attribute__((unused)),
	const char*file,
	int line,
	const char*fn,
	const char*subs,
	unsigned int sev,
	const char*format,
	va_list args
){
	lv_log_level_t lvl;
	char buf[BUFSIZ];
	switch(sev){
		case LLOG_DEBUG:lvl=LV_LOG_LEVEL_TRACE;break;
		case LLOG_INFO:lvl=LV_LOG_LEVEL_INFO;break;
		case LLOG_NOTICE:lvl=LV_LOG_LEVEL_INFO;break;
		case LLOG_WARNING:lvl=LV_LOG_LEVEL_WARN;break;
		case LLOG_ERROR:lvl=LV_LOG_LEVEL_ERROR;break;
		case LLOG_CRITICAL:lvl=LV_LOG_LEVEL_ERROR;break;
		case LLOG_ALERT:lvl=LV_LOG_LEVEL_ERROR;break;
		case LLOG_FATAL:lvl=LV_LOG_LEVEL_ERROR;break;
		default:lvl=LV_LOG_LEVEL_USER;
	}
	memset(buf,0,sizeof(buf));
	vsnprintf(buf,sizeof(buf)-1,format,args);
	_lv_log_add(lvl,file,line,fn,"%s: %s",subs,buf);
}

static void write_cb(
	struct tsm_vte*vte,
	const char*u8,
	size_t len,
	void*data
){
	lv_obj_t*tv=data;
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	if(!vte||!tv||!term||term->vte!=vte)return;
	if(term->write_cb)term->write_cb(tv,u8,len);
}

static void osc_cb(
	struct tsm_vte*vte,
	const char*u8,
	size_t len,
	void*data
){
	lv_obj_t*tv=data;
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	if(!vte||!tv||!term||term->vte!=vte)return;
	if(term->osc_cb)term->osc_cb(tv,u8,len);
}

static void virt_input_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e==LV_EVENT_DELETE)return;
	lv_obj_t*tv=lv_obj_get_user_data(obj);
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	if(obj!=term->virt_input)return;
	if(e==LV_EVENT_VALUE_CHANGED){
		const char*txt=lv_textarea_get_text(term->virt_input);
		size_t len=strlen(txt);
		for(size_t i=0;i<len;i++)if(tsm_vte_handle_keyboard(
			term->vte,0,txt[i],term->mods,txt[i]
		))tsm_screen_sb_reset(term->screen);
		if(len>0)lv_textarea_set_text(term->virt_input,"");
	}
	lv_termview_update(tv);
}

struct tsm_screen*lv_termview_get_screen(lv_obj_t*tv){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	return term->screen;
}

struct tsm_vte*lv_termview_get_vte(lv_obj_t*tv){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	return term->vte;
}

lv_obj_t*lv_termview_get_virtual_input(lv_obj_t*tv){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	return term->virt_input;
}

void lv_termview_set_osc_cb(
	lv_obj_t*tv,
	termview_osc_cb cb
){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	term->osc_cb=cb;
}

void lv_termview_set_write_cb(
	lv_obj_t*tv,
	termview_write_cb cb
){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	term->write_cb=cb;
}

void lv_termview_set_resize_cb(
	lv_obj_t*tv,
	termview_resize_cb cb
){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	term->resize_cb=cb;
}

void lv_termview_set_mods(
	lv_obj_t*tv,
	uint32_t mods
){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	term->mods=mods;
}

void lv_termview_set_max_scroll_buffer_size(
	lv_obj_t*tv,
	uint32_t max_sb
){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	term->max_sb=max_sb;
	tsm_screen_set_max_sb(term->screen,max_sb);
}

void lv_termview_set_font(lv_obj_t*tv,lv_font_t*font){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	if(!font||font->line_height<=0)return;
	term->glyph_height=font->line_height;
	term->glyph_width=font->line_height/2;
	term->font_reg=font;
	term->font_bold=font;
	term->font_ital=font;
	term->font_bold_ital=font;
	term->cust_font=true;
}

void lv_termview_set_font_regular(lv_obj_t*tv,lv_font_t*font){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	if(!font||font->line_height<=0)return;
	term->glyph_height=font->line_height;
	term->glyph_width=font->line_height/2;
	term->font_reg=font;
	term->cust_font=true;
}

void lv_termview_set_font_bold(lv_obj_t*tv,lv_font_t*font){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	if(!font||font->line_height<=0)return;
	term->font_bold=font;
	term->cust_font=true;
}

void lv_termview_set_font_italic(lv_obj_t*tv,lv_font_t*font){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	if(!font||font->line_height<=0)return;
	term->font_ital=font;
	term->cust_font=true;
}

void lv_termview_set_font_bold_italic(lv_obj_t*tv,lv_font_t*font){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	if(!font)return;
	term->font_bold_ital=font;
	term->cust_font=true;
}

void lv_termview_update(lv_obj_t*tv){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	term->age=tsm_screen_draw(
		term->screen,
		term_draw_cell,
		tv
	);
}

uint32_t lv_termview_get_cols(lv_obj_t*tv){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	return term->cols;
}

uint32_t lv_termview_get_rows(lv_obj_t*tv){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	return term->rows;
}

void lv_termview_input(lv_obj_t*tv,const char*str){
	tsm_vte_input(
		lv_termview_get_vte(tv),
		str,strlen(str)
	);
	lv_termview_update(tv);
}

void lv_termview_input_fmt(lv_obj_t*tv,const char*fmt,...){
	va_list args;
	va_start(args,fmt);
	char*cs=_lv_txt_set_text_vfmt(fmt,args);
	va_end(args);
	lv_termview_input(tv,cs);
	lv_mem_free(cs);
}

void lv_termview_line_printf(lv_obj_t*tv,const char*fmt,...){
	lv_termview_ext_t*term=lv_obj_get_ext_attr(tv);
	va_list args;
	va_start(args,fmt);
	char*cs=_lv_txt_set_text_vfmt(fmt,args);
	va_end(args);
	tsm_vte_input(term->vte,"\r",1);
	if(tsm_screen_get_cursor_x(term->screen)>0)
		tsm_vte_input(term->vte,"\n",1);
        tsm_vte_input(term->vte,cs,strlen(cs));
	tsm_vte_input(term->vte,"\r\n",2);
	lv_termview_update(tv);
}

lv_obj_t*lv_termview_create(lv_obj_t*par,const lv_obj_t*copy){
	if(copy){
		LV_LOG_ERROR("unsupported terminal view copy");
		return NULL;
	}
	lv_obj_t*ta=NULL,*canvas=NULL;
	LV_LOG_TRACE("terminal view create started");
	canvas=lv_canvas_create(par,copy);
	LV_ASSERT_MEM(canvas);
	if(!canvas)goto fail;
	ta=lv_textarea_create(canvas,copy);
	LV_ASSERT_MEM(ta);
	if(!ta)goto fail;
	lv_termview_ext_t*ext=lv_obj_allocate_ext_attr(
		canvas,
		sizeof(lv_termview_ext_t)
	);
	LV_ASSERT_MEM(ext);
	if(!ext)goto fail;
	ext->sig=lv_obj_get_signal_cb(canvas);
	if(tsm_screen_new(
		&ext->screen,
		NULL,NULL
	)!=0)goto fail;
	if(tsm_vte_new(
		&ext->vte,ext->screen,
		write_cb,canvas,
		log_cb,NULL
	)<0)goto fail;
	ext->virt_input=ta,ext->cust_font=false;
	ext->glyph_height=0,ext->glyph_width=0;
	ext->cols=0,ext->rows=0,ext->age=0;
	ext->buffer=NULL,ext->mem_size=0;
	ext->mods=0,ext->drag_y_last=0;
	ext->resize_cb=NULL;
	ext->write_cb=NULL;
	ext->osc_cb=NULL;
	lv_obj_t*p=lv_obj_get_parent(canvas);
	lv_obj_set_size(
		canvas,
		lv_obj_get_width_fit(p),
		lv_obj_get_height_fit(p)
	);
	lv_textarea_set_text(ta,"");
	lv_obj_set_event_cb(ta,virt_input_cb);
	lv_obj_set_user_data(ta,canvas);
	lv_obj_set_pos(
		ta,
		-lv_obj_get_width(ta),
		-lv_obj_get_height(ta)
	);
	lv_obj_set_click(canvas,true);
	lv_obj_set_signal_cb(
		canvas,
		lv_termview_signal
	);
	tsm_vte_set_osc_cb(
		ext->vte,
		osc_cb,
		canvas
	);
	lv_termview_resize(canvas);
	LV_LOG_INFO("terminal view created");
	return canvas;
	fail:
	if(canvas)lv_obj_del(canvas);
	if(ta)lv_obj_del(ta);
	return NULL;
}
#endif
#endif
