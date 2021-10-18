/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _KLOGLEVEL_H
#define _KLOGLEVEL_H

// syslog level
#define SYSLOG_ACTION_CLOSE         0
#define SYSLOG_ACTION_OPEN          1
#define SYSLOG_ACTION_READ          2
#define SYSLOG_ACTION_READ_ALL      3
#define SYSLOG_ACTION_READ_CLEAR    4
#define SYSLOG_ACTION_CLEAR         5
#define SYSLOG_ACTION_CONSOLE_OFF   6
#define SYSLOG_ACTION_CONSOLE_ON    7
#define SYSLOG_ACTION_CONSOLE_LEVEL 8
#define SYSLOG_ACTION_SIZE_UNREAD   9
#define SYSLOG_ACTION_SIZE_BUFFER   10

// kernel ring buffer level
#define KERN_EMERG                  0
#define KERN_ALERT                  1
#define KERN_CRIT                   2
#define KERN_ERR                    3
#define KERN_WARNING                4
#define KERN_NOTICE                 5
#define KERN_INFO                   6
#define KERN_DEBUG                  7


// default level
#define DEFAULT_KERN_LEVEL KERN_WARNING
#endif
