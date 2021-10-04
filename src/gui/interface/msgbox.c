#ifdef ENABLE_GUI
#include"gui.h"
#include"activity.h"
#include"logger.h"
#include"tools.h"
#include"msgbox.h"

static void msg_click(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_DELETE)guiact_do_back();
	else if(e==LV_EVENT_VALUE_CHANGED){
		lv_obj_t*o=obj;
		do{
			if(!lv_debug_check_obj_type(o,"lv_msgbox"))continue;
			uint16_t id=lv_msgbox_get_active_btn(obj);
			const char*btn=lv_msgbox_get_active_btn_text(obj);
			msgbox_callback cb=o->user_data;
			if(cb&&cb(id,btn))return;
			break;
		}while((o=lv_obj_get_parent(o)));
		lv_msgbox_start_auto_close(obj,0);
	}
}

static int msgbox_draw(struct gui_activity*act){
	lv_obj_t*o;
	struct msgbox*box=act->args;
	switch(box->mode){
		case MODE_OK:o=lv_create_ok_msgbox(act->page,msg_click,box->title);break;
		case MODE_YESNO:o=lv_create_yesno_msgbox(act->page,msg_click,box->title);break;
		case MODE_CUSTOM:o=lv_create_msgbox(act->page,box->buttons,msg_click,box->title);break;
		default:return 1;
	}
	lv_obj_set_user_data(o,box->callback);
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

void msgbox_create(
	char*content,
	enum msgbox_mode mode,
	msgbox_callback callback,
	const char**buttons
){
	struct msgbox msg={
		.title=content,
		.mode=mode,
		.callback=callback,
		.buttons=buttons
	};
	guiact_start_activity(&guireg_msgbox,&msg);
}

#endif
