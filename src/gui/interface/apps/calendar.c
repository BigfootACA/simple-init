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
static lv_obj_t*cal=NULL;

static const char**get_month_name(){
	static const char*mn[12]={0};
	static const char*_mn[12]={
		"January","February","March","April","May","June","July",
		"August","September","October","November","December"
	};
	if(!mn[0])for(int i=0;i<12;i++)mn[i]=_(_mn[i]);
	return mn;
}

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

static int calendar_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,cal);
	return 0;
}

static int calendar_lost_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_remove_obj(cal);
	return 0;
}

static int calendar_draw(struct gui_activity*act){
	lv_calendar_date_t d;
	struct tm*tt;
	time_t ct;
	ct=time(NULL);
	tt=localtime(&ct);
	if(tt)d.year=tt->tm_year+1900,d.month=tt->tm_mon+1,d.day=tt->tm_mday;
	cal=lv_calendar_create(act->page,NULL);
	lv_obj_set_style_local_border_width(cal,LV_CALENDAR_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(cal,LV_CALENDAR_PART_BG,LV_STATE_FOCUSED,0);
	lv_obj_set_style_local_border_width(cal,LV_CALENDAR_PART_BG,LV_STATE_PRESSED,0);
	lv_calendar_set_month_names(cal,get_month_name());
	lv_calendar_set_day_names(cal,get_day_name());
	lv_calendar_set_today_date(cal,&d);
	lv_calendar_set_showed_date(cal,&d);
	lv_obj_align(cal,NULL,LV_ALIGN_CENTER,0,0);
	return 0;
}

struct gui_register guireg_calendar={
	.name="calendar",
	.title="Calendar",
	.icon="calendar.png",
	.show_app=true,
	.draw=calendar_draw,
	.get_focus=calendar_get_focus,
	.lost_focus=calendar_lost_focus,
	.back=true,
	.mask=true,
};
#endif
