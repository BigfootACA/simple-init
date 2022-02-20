/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<string.h>
#include"str.h"
#include"kloglevel.h"
#include"logger_internal.h"

char*logger_level2string(enum log_level level){
	switch(level){
		case LEVEL_DEBUG:return   "DEBUG";
		case LEVEL_INFO:return    "INFO";
		case LEVEL_NOTICE:return  "NOTICE";
		case LEVEL_WARNING:return "WARN";
		case LEVEL_ERROR:return   "ERROR";
		case LEVEL_CRIT:return    "CRIT";
		case LEVEL_ALERT:return   "ALERT";
		case LEVEL_EMERG:return   "EMERG";
		case LEVEL_VERBOSE:return "VERB";
		default:return            "?????";
	}
}

enum log_level logger_parse_level(const char*v){
	#define CS (const char*[])
	if(!v)return 0;
	if(     fuzzy_cmps(v,CS{"8","verbose","verb"   ,NULL}))return LEVEL_VERBOSE;
	else if(fuzzy_cmps(v,CS{"7","debug","dbg"      ,NULL}))return LEVEL_DEBUG;
	else if(fuzzy_cmps(v,CS{"6","info","inf"       ,NULL}))return LEVEL_INFO;
	else if(fuzzy_cmps(v,CS{"5","notice"           ,NULL}))return LEVEL_NOTICE;
	else if(fuzzy_cmps(v,CS{"4","warning"          ,NULL}))return LEVEL_WARNING;
	else if(fuzzy_cmps(v,CS{"3","error"            ,NULL}))return LEVEL_ERROR;
	else if(fuzzy_cmps(v,CS{"2","critical"         ,NULL}))return LEVEL_CRIT;
	else if(fuzzy_cmps(v,CS{"1","alert","alrt"     ,NULL}))return LEVEL_ALERT;
	else if(fuzzy_cmps(v,CS{"0","emgcy","emergency",NULL}))return LEVEL_EMERG;
	else return 0;
}

int logger_level2klevel(enum log_level level){
	switch(level){
		case LEVEL_VERBOSE:return KERN_DEBUG;
		case LEVEL_DEBUG:return   KERN_DEBUG;
		case LEVEL_INFO:return    KERN_INFO;
		case LEVEL_NOTICE:return  KERN_NOTICE;
		case LEVEL_WARNING:return KERN_WARNING;
		case LEVEL_ERROR:return   KERN_ERR;
		case LEVEL_CRIT:return    KERN_CRIT;
		case LEVEL_ALERT:return   KERN_ALERT;
		case LEVEL_EMERG:return   KERN_EMERG;
		default:return            KERN_NOTICE;
	}
}

enum log_level logger_klevel2level(int level){
	switch(level){
		case KERN_DEBUG:return   LEVEL_DEBUG;
		case KERN_INFO:return    LEVEL_INFO;
		case KERN_NOTICE:return  LEVEL_NOTICE;
		case KERN_WARNING:return LEVEL_WARNING;
		case KERN_ERR:return     LEVEL_ERROR;
		case KERN_CRIT:return    LEVEL_CRIT;
		case KERN_ALERT:return   LEVEL_ALERT;
		case KERN_EMERG:return   LEVEL_EMERG;
		default:return          LEVEL_NOTICE;
	}
}
