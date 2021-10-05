#ifdef ENABLE_GUI
#include<stdlib.h>
#include"gui.h"
#include"activity.h"
#include"logger.h"
#include"tools.h"
#include"msgbox.h"

static void msg_click(lv_obj_t*obj,lv_event_t e){
	struct msgbox*cb=NULL;
	if(e==LV_EVENT_DELETE)guiact_do_back();
	else if(e==LV_EVENT_VALUE_CHANGED){
		lv_obj_t*o=obj;
		do{
			if(!lv_debug_check_obj_type(o,"lv_msgbox"))continue;
			uint16_t id=lv_msgbox_get_active_btn(obj);
			const char*btn=lv_msgbox_get_active_btn_text(obj);
			cb=o->user_data;
			if(cb&&cb->callback&&cb->callback(id,btn))return;
			break;
		}while((o=lv_obj_get_parent(o)));
		if(cb)free(cb);
		lv_obj_del_async(obj);
	}
}

static int msgbox_draw(struct gui_activity*act){
	lv_obj_t*o;
	struct msgbox*box=act->args;
	switch(box->mode){
		case MODE_OK:o=lv_create_ok_msgbox(act->page,msg_click,box->text);break;
		case MODE_YESNO:o=lv_create_yesno_msgbox(act->page,msg_click,box->text);break;
		case MODE_CUSTOM:o=lv_create_msgbox(act->page,box->buttons,msg_click,box->text);break;
		default:return 1;
	}
	lv_obj_set_user_data(o,box);
	return 0;
}

static int msgbox_get_focus(struct gui_activity*d){
	lv_group_add_msgbox(gui_grp,d->page,true);
	return 0;
}

static int msgbox_lost_focus(struct gui_activity*d){
	lv_group_remove_msgbox(d->page);
	return 0;
}

struct gui_register guireg_msgbox={
	.name="msgbox",
	.title="Message Box",
	.show_app=false,
	.draw=msgbox_draw,
	.get_focus=msgbox_get_focus,
	.lost_focus=msgbox_lost_focus,
	.back=true,
	.mask=true,
};

static void msgbox_cb(lv_task_t*t){
	guiact_start_activity(&guireg_msgbox,t->user_data);
}

static void _msgbox_create(
	enum msgbox_mode mode,
	msgbox_callback callback,
	const char**buttons,
	const char*content,
	va_list va
){
	struct msgbox*msg=malloc(sizeof(struct msgbox));
	if(!msg)return;
	vsnprintf(msg->text,sizeof(msg->text)-1,_(content),va);
	msg->mode=mode;
	msg->callback=callback;
	msg->buttons=buttons;
	lv_task_once(lv_task_create(msgbox_cb,0,LV_TASK_PRIO_LOWEST,msg));
}

void msgbox_create(
	enum msgbox_mode mode,
	msgbox_callback callback,
	const char**buttons,
	const char*content,
	...
){
	va_list va;
	va_start(va,content);
	_msgbox_create(mode,callback,buttons,content,va);
	va_end(va);
}

void msgbox_create_yesno(msgbox_callback callback,const char*content,...){
	va_list va;
	va_start(va,content);
	_msgbox_create(MODE_YESNO,callback,NULL,content,va);
	va_end(va);
}

void msgbox_create_ok(msgbox_callback callback,const char*content,...){
	va_list va;
	va_start(va,content);
	_msgbox_create(MODE_OK,callback,NULL,content,va);
	va_end(va);
}

void msgbox_alert(const char*content,...){
	va_list va;
	va_start(va,content);
	_msgbox_create(MODE_OK,NULL,NULL,content,va);
	va_end(va);
}

#endif
