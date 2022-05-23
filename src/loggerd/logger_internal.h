/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _LOGGER_INTERNAL_H
#define _LOGGER_INTERNAL_H
#include<stdio.h>
#include<sys/socket.h>
#include"list.h"
#include"logger.h"

// logger packet magic
#define LOGD_MAGIC0 0xEF
#define LOGD_MAGIC1 0x88

// logger operation
enum log_oper{
	LOG_OK       =0xAF00,
	LOG_FAIL     =0xAF01,
	LOG_QUIT     =0xAF02,
	LOG_ADD      =0xAF03,
	LOG_OPEN     =0xAF04,
	LOG_CLOSE    =0xAF05,
	LOG_CLEAR    =0xAF06,
	LOG_LISTEN   =0xAF07,
	LOG_KLOG     =0xAF08,
	LOG_SYSLOG   =0xAF09,
	LOG_CONSOLE  =0xAF0A,
};

// logger message packet
struct log_msg{
	unsigned char magic0,magic1;
	enum log_oper oper;
	union{
		int code;
		struct log_item log;
		char string[sizeof(struct log_item)];
	}data;
};

// logger output handle
typedef int on_log(char*,struct log_item*);

// logger output
struct logger{
	char*name;
	bool flushed;
	enum log_level min_level;
	on_log*logger;
	bool enabled;
};

// src/loggerd/internal.c: logger output list
extern list*loggers;

// src/loggerd/server.c: logger server thread
extern int loggerd_thread(int fd);

// src/loggerd/file_logger.c: file or stdio logger output
extern int file_logger(char*name,struct log_item *log);

// src/loggerd/syslog_logger.c: syslog logger output
extern int syslog_logger(char*name,struct log_item *log);

// src/loggerd/printk_logger.c: kernel ring buffer logger output
extern int printk_logger(char*name,struct log_item *log);

// src/loggerd/file_logger.c: open log file
extern int open_log_file(char*path);

// src/loggerd/file_logger.c: close log file
extern void close_log_file(char*path);

// src/loggerd/file_logger.c: close all openned log file
extern void close_all_file(void);

// src/loggerd/internal.c: free loggers
extern void logger_internal_clean(void);

// src/loggerd/internal.c: add new logger
extern int logger_internal_add(char*name,enum log_level min_level,on_log log);

// src/loggerd/internal.c: add raw log
extern int logger_internal_write(struct log_item*log);

// src/loggerd/internal.c: add log with level, tag, content
extern int logger_internal_print(enum log_level level,char*tag,char*content);

// src/loggerd/internal.c: add log with level, tag, formatted content
extern int logger_internal_printf(enum log_level level,char*tag,const char*fmt,...) __attribute__((format(printf,3,4)));

// src/loggerd/internal.c: turn on or off logger output
extern int logger_internal_set(char*name,bool enabled);

// src/loggerd/internal.c: set logger output level
extern int logger_internal_set_level(char*name,enum log_level level);

// src/loggerd/internal.c: init a log packaet
extern void logger_internal_init_msg(struct log_msg*msg,enum log_oper oper);

// src/loggerd/internal.c: check log packaet magic
extern bool logger_internal_check_magic(struct log_msg*msg);

// src/loggerd/internal.c: read a log packet
extern int logger_internal_read_msg(int fd,struct log_msg*buff);

// src/loggerd/internal.c: send a return code packet
extern int logger_internal_send_code(int fd,enum log_oper oper,int code);

// src/loggerd/internal.c: send a string log packet
extern int logger_internal_send_string(int fd,enum log_oper oper,char*string);

// src/loggerd/buffer.c: convert operation to a readable string
extern char*logger_oper2string(enum log_oper oper);

// src/loggerd/buffer.c: convert log_item to log_buff
extern struct log_buff*logger_internal_item2buff(struct log_item*log);

// src/loggerd/buffer.c: convert log_buff to log_item
extern struct log_item*logger_internal_buff2item(struct log_buff*log);

// src/loggerd/buffer.c: add log to buffer
extern int logger_internal_buffer_push(struct log_item*log);

// src/loggerd/buffer.c: free log_buff
extern int logger_internal_free_buff(void*d);

// src/loggerd/buffer.c: clean log buffers
extern void clean_log_buffers(void);

// src/loggerd/buffer.c: flush buffer to logger
#ifdef ENABLE_UEFI
extern void flush_buffer();
#else
extern void flush_buffer(struct logger*log);
#endif

#ifdef ENABLE_UEFI
// src/loggerd/client.c: write file logger output
extern void logger_out_write(char*buff);
#endif

// src/loggerd/klog.c: read all kmesg to buffer and read new kmsg to logger
extern int init_kmesg(void);

// src/loggerd/syslog.c: read all kmesg to buffer and read new kmsg to logger
extern int init_syslog(void);
#endif
