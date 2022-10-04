/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"confd.h"
#include"gui/msgbox.h"
#include"gui/activity.h"

static bool conf_save_cb(
	uint16_t id,
	const char*btn __attribute__((unused)),
	void*user_data
){
	char*path=user_data;
	if(!path)return true;
	if(id==0){
		int r=0;
		r=confd_save_file(path);
		if(r<0)r=-r;
		if(r!=0)msgbox_alert(
			"Save config failed: %s",
			strerror(r)
		);
	}
	return false;
}

static int conf_save_draw(struct gui_activity*act){
	if(!act||!act->args)return -1;
	msgbox_set_user_data(msgbox_create_yesno(
		conf_save_cb,
		"Save config to '%s'?",
		(char*)act->args
	),act->args);
	return -10;
}

struct gui_register guireg_conf_save={
	.name="config-save",
	.title="Save Config",
	.show_app=false,
	.open_file=true,
	.open_regex=(char*[]){
		".*\\.cfg$",
		".*\\.json$",
		".*\\.jsn$",
		".*\\.xml$",
		NULL
	},
	.draw=conf_save_draw,
};
#endif
