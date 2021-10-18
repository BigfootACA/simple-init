/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<string.h>
#include<syslog.h>
#include"logger.h"
#include"defines.h"

int syslog_logger(char*name __attribute__((unused)),struct log_item *log){
	if(!log->time)ERET(EFAULT);
	openlog(log->tag,LOG_CONS,LOG_DAEMON);
	syslog(logger_level2klevel(log->level),"%s",(char*)log->content);
	closelog();
	return (int)strlen(log->content);
}
