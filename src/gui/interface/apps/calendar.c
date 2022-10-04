/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_GUI
#include<time.h>
#include"gui.h"
#include"gui/activity.h"

static const char**get_day_name(){
	static const char*dn[7]={0};
	#if LV_CALENDAR_WEEK_STARTS_MONDAY != 0
	static const char*_dn[7]={"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
	#else
	static const char*_dn[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	#endif
	if(!dn[0])for(int i=0;i<7;i++)dn[i]=_(_dn[i]);
	return dn;
}

static int calendar_get_focus(struct gui_activity*d){
	if(d->data)lv_group_add_obj(gui_grp,d->data);
	return 0;
}

static int calendar_lost_focus(struct gui_activity*d){
	if(d->data)lv_group_remove_obj(d->data);
	return 0;
}

static int calendar_cleanup(struct gui_activity*d){
	d->data=NULL;
	return 0;
}

static int calendar_draw(struct gui_activity*act){
	struct tm*tt;
	time_t ct;
	ct=time(NULL);
	tt=localtime(&ct);
	lv_obj_t*cal=lv_calendar_create(act->page);
	lv_obj_set_style_max_width(cal,lv_pct(90),0);
	lv_obj_set_style_max_height(cal,lv_pct(90),0);
	lv_obj_set_style_min_width(cal,5*gui_dpi/2,0);
	lv_obj_set_style_min_height(cal,5*gui_dpi/2,0);
	lv_calendar_set_day_names(cal,get_day_name());
	lv_calendar_set_today_date(cal,tt->tm_year+1900,tt->tm_mon+1,tt->tm_mday);
	lv_calendar_set_showed_date(cal,tt->tm_year+1900,tt->tm_mon+1);
	lv_obj_center(cal);
	act->data=cal;
	return 0;
}

struct gui_register guireg_calendar={
	.name="calendar",
	.title="Calendar",
	.show_app=true,
	.draw=calendar_draw,
	.quiet_exit=calendar_cleanup,
	.get_focus=calendar_get_focus,
	.lost_focus=calendar_lost_focus,
	.back=true,
	.mask=true,
};
#endif
