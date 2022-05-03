/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<stdbool.h>
#include"gui.h"
#include"logger.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "pointer-test"

struct pointer_indev{
	lv_point_t last;
	lv_indev_t*indev;
	lv_indev_data_t data;
};

struct pointer_test{
	lv_task_t*tsk;
	lv_obj_t*canvas;
	lv_color_t*buffer;
	size_t size;
	lv_coord_t w,h;
	list*indevs;
};

static void task_cb(lv_task_t*tsk){
	bool mtr;
	list*l=NULL;
	lv_draw_line_dsc_t dsc;
	struct pointer_test*pv=tsk->user_data;
	if(!pv||!pv->buffer||!pv->indevs)return;
	lv_draw_line_dsc_init(&dsc);
	dsc.width=gui_dpi/80;
	if((l=list_first(pv->indevs)))do{
		LIST_DATA_DECLARE(p,l,struct pointer_indev*);
		if(p->indev->driver.type!=LV_INDEV_TYPE_POINTER)continue;
		do{
			mtr=_lv_indev_read(p->indev,&p->data);
			if(p->data.point.x<0||p->data.point.x>pv->w)continue;
			if(p->data.point.y<0||p->data.point.y>pv->h)continue;
			if(p->last.x!=0&&p->last.y!=0){
				switch(p->data.state){
					case LV_INDEV_STATE_PR:dsc.color=LV_COLOR_RED;break;
					case LV_INDEV_STATE_REL:dsc.color=LV_COLOR_BLUE;break;
					default:dsc.color=LV_COLOR_GRAY;break;
				}
				lv_canvas_draw_line(
					pv->canvas,
					(lv_point_t[]){p->last,p->data.point},
					2,&dsc
				);
			}
			p->last.x=p->data.point.x;
			p->last.y=p->data.point.y;
		}while(mtr);
	}while((l=l->next));
}

static void block_indev(struct pointer_test*pv,bool state){
	lv_indev_t*indev=NULL;
	struct pointer_indev in;
	list_free_all_def(pv->indevs);
	pv->indevs=NULL;
	while((indev=lv_indev_get_next(indev))){
		if(indev->driver.type!=LV_INDEV_TYPE_POINTER)continue;
		if(state){
			memset(&in,0,sizeof(in));
			in.indev=indev;
			list_obj_add_new_dup(&pv->indevs,&in,sizeof(in));
		}
		indev->proc.disabled=state;
	}
}

static int do_clean(struct gui_activity*act){
	struct pointer_test*pv=act->data;
	if(!pv)return 0;
	if(pv->tsk)lv_task_del(pv->tsk);
	block_indev(pv,false);
	memset(pv,0,sizeof(struct pointer_test));
	free(pv);
	act->data=NULL;
	return 0;
}

static int pointer_test_get_focus(struct gui_activity*act){
	struct pointer_test*pv=act->data;
	if(pv)block_indev(pv,true);
	return 0;
}

static int pointer_test_lost_focus(struct gui_activity*act){
	struct pointer_test*pv=act->data;
	if(pv)block_indev(pv,false);
	return 0;
}

static int pointer_test_init(struct gui_activity*act){
	struct pointer_test*pv=malloc(sizeof(struct pointer_test));
	if(!pv)return -1;
	memset(pv,0,sizeof(struct pointer_test));
	act->data=pv;
	return 0;
}

static int pointer_test_resize(struct gui_activity*act){
	size_t size;
	lv_canvas_ext_t*ext;
	struct pointer_test*pv=act->data;
	if(!pv||!(ext=lv_obj_get_ext_attr(pv->canvas)))return -1;
	lv_obj_set_pos(pv->canvas,0,0);
	lv_obj_set_size(pv->canvas,act->w,act->h);
	size=LV_CANVAS_BUF_SIZE_TRUE_COLOR(act->w,act->h);
	if(size==pv->size)return 0;
	if(pv->buffer){
		lv_img_cache_invalidate_src(
			lv_canvas_get_img(pv->canvas)
		);
		free(pv->buffer);
	}
	if(!(pv->buffer=malloc(size)))
		return trlog_error(-1,"cannot allocate buffer for canvas");
	memset(pv->buffer,0xFF,size);
	ext->dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
	ext->dsc.header.w  = act->w;
	ext->dsc.header.h  = act->h;
	ext->dsc.data      = (void*)pv->buffer;
	lv_img_set_src(pv->canvas,&ext->dsc);
	pv->size=size,pv->w=act->w,pv->h=act->h;
	return 0;
}

static bool msgbox_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	struct pointer_test*pv=user_data;
	switch(id){
		case 0:
			if(pv->tsk)break;
			pv->tsk=lv_task_create(
				task_cb,
				LV_INDEV_DEF_READ_PERIOD,
				LV_TASK_PRIO_HIGH,
				pv
			);
		break;
		case 1:
			guiact_do_back();
			guiact_do_back();
		break;
	}
	return false;
}

static int pointer_test_draw(struct gui_activity*act){
	struct pointer_test*pv=act->data;
	if(!pv)return -1;
	pv->canvas=lv_canvas_create(act->page,NULL);
	msgbox_set_user_data(msgbox_create_yesno(
		msgbox_cb,
		"This app will have exclusive pointer input,"
		" are you sure to continue?"
	),pv);
	return 0;
}

struct gui_register guireg_pointer_test={
	.name="pointer-test",
	.title="Pointer Test",
	.icon="pointer.svg",
	.open_file=false,
	.show_app=true,
	.draw=pointer_test_draw,
	.init=pointer_test_init,
	.get_focus=pointer_test_get_focus,
	.lost_focus=pointer_test_lost_focus,
	.resize=pointer_test_resize,
	.quiet_exit=do_clean,
	.back=true,
	.full_screen=true,
};
#endif
