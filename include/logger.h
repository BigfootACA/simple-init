/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _LOGGER_H
#define _LOGGER_H
#include<time.h>
#include<stdbool.h>
#include"pathnames.h"
#define DEFAULT_LOGGER _PATH_RUN"/loggerd.sock"

// logger level
enum log_level{
	LEVEL_VERBOSE=0xAE00,
	LEVEL_DEBUG  =0xAE01,
	LEVEL_INFO   =0xAE02,
	LEVEL_NOTICE =0xAE04,
	LEVEL_WARNING=0xAE08,
	LEVEL_ERROR  =0xAE10,
	LEVEL_CRIT   =0xAE20,
	LEVEL_ALERT  =0xAE40,
	LEVEL_EMERG  =0xAE80
};

// log item
struct log_item{
	time_t time;
	pid_t pid;
	enum log_level level;
	char tag[64];
	char content[16384-
		64-
		sizeof(enum log_level)-
		sizeof(time_t)-
		sizeof(pid_t)
	];
};

// log storage item
struct log_buff{
	enum log_level level;
	char*tag,*content;
	time_t time;
	pid_t pid;
};

#ifdef _LIST_H
// src/loggerd/buffer.c: log storage
extern list*logbuffer;
#endif

#ifndef ENABLE_UEFI
// src/loggerd/client.c: current logfd
extern int logfd;

// src/loggerd/client.c: set logger output fd
extern int set_logfd(int fd);

// src/loggerd/client.c: close current logfd
extern void close_logfd(void);

// src/loggerd/client.c: open and set logger output fd
extern int open_file_logfd(char*path);

// src/loggerd/client.c: connect and set logger output fd
extern int open_socket_logfd(char*path);

// src/loggerd/client.c: controll loggerd add new listen
extern int logger_listen(char*file);

// src/loggerd/client.c: controll loggerd open new log file
extern int logger_open(char*file);

// src/loggerd/client.c: controll loggerd exit
extern int logger_exit(void);

// src/loggerd/client.c: notify loggerd kernel log now available
extern int logger_klog(void);

// src/loggerd/client.c: start syslog forwarder
extern int logger_syslog(void);

// src/loggerd/client.c: reopen all active consoles
extern int logger_open_console(void);

// src/loggerd/client.c: launch loggerd
extern int start_loggerd(pid_t*p);
#else
static inline int set_logfd(int fd __attribute__((unused))){return -1;}
static inline void close_logfd(void){};
static inline int open_file_logfd(char*path __attribute__((unused))){return -1;}
static inline int open_socket_logfd(char*path __attribute__((unused))){return -1;}
static inline int logger_listen(char*file __attribute__((unused))){return -1;}
static inline int logger_open(char*file __attribute__((unused))){return -1;}
static inline int logger_exit(void){return -1;}
static inline int logger_klog(void){return -1;}
static inline int logger_syslog(void){return -1;}
static inline int start_loggerd(int*p __attribute__((unused))){return -1;}
extern void logger_set_console(bool enabled);
extern void logger_init(void);
#endif

// src/loggerd/client.c: set local logger level
extern void logger_set_level(enum log_level level);

// src/loggerd/client.c: send raw log
extern int logger_write(struct log_item*log);

// src/loggerd/client.c: send log with level, tag, content
extern int logger_print(enum log_level level,char*tag,char*content);

// src/loggerd/client.c: send log with level, tag, formatted content
extern int logger_printf(enum log_level level,char*tag,const char*fmt,...) __attribute__((format(printf,3,4)));

// src/loggerd/client.c: send log with level, tag, formatted content, strerror
extern int logger_perror(enum log_level level,char*tag,const char*fmt,...) __attribute__((format(printf,3,4)));

// src/loggerd/client.c: send log with level, tag, formatted content and return e
extern int return_logger_printf(enum log_level level,int e,char*tag,const char*fmt,...) __attribute__((format(printf,4,5)));

// src/loggerd/client.c: send log with level, tag, formatted content, strerror and return e
extern int return_logger_perror(enum log_level level,int e,char*tag,const char*fmt,...) __attribute__((format(printf,4,5)));

// src/loggerd/lib.c: convert log_level to string
extern char*logger_level2string(enum log_level level);

// src/loggerd/lib.c: convert log_level to kernel level
extern int logger_level2klevel(enum log_level level);

// src/loggerd/lib.c: convert kernel level to log_level
extern enum log_level logger_klevel2level(int level);

// src/loggerd/lib.c: parse log_level from a string
extern enum log_level logger_parse_level(const char*v);

#include"logtag.h"
#endif
