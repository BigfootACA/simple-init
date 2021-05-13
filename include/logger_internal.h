#ifndef _LOGGER_INTERNAL_H
#define _LOGGER_INTERNAL_H
#include<stdio.h>
#include"logger.h"
#include"list.h"

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
	LOG_CLEAR    =0xAF07,
	LOG_LISTEN   =0xAF09,
	LOG_KLOG     =0xAF06,
};

// logger message packet
struct log_msg{
	unsigned char magic0,magic1;
	enum log_oper oper;
	size_t size;
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

// log storage item
struct log_buff{
	enum log_level level;
	char*tag,*content;
	time_t time;
	pid_t pid;
};

// src/loggerd/logger_internal.c: logger output list
extern list*loggers;

// src/loggerd/logger_buffer.c: log storage
extern list*logbuffer;

// src/loggerd/logger_server.c: logger server thread
extern int loggerd_thread(int fd);

// src/loggerd/file_logger.c: file or stdio logger output
extern int file_logger(char*name,struct log_item *log);

// src/loggerd/syslog_logger.c: syslog logger output
extern int syslog_logger(char*name,struct log_item *log);

// src/loggerd/printk_logger.c: kernel ring buffer logger output
extern int printk_logger(char*name,struct log_item *log);

// src/loggerd/file_logger.c: open log file
extern FILE*open_log_file(char*path);

// src/loggerd/file_logger.c: close log file
extern void close_log_file(char*path);

// src/loggerd/file_logger.c: close all openned log file
extern void close_all_file();

// src/loggerd/logger_internal.c: free loggers
extern void internal_clean_loggers();

// src/loggerd/logger_internal.c: add new logger
extern int internal_add_logger(char*name,enum log_level min_level,on_log log);

// src/loggerd/logger_internal.c: add raw log
extern int internal_logger_write(struct log_item*log);

// src/loggerd/logger_internal.c: add log with level, tag, content
extern int internal_logger_print(enum log_level level,char*tag,char*content);

// src/loggerd/logger_internal.c: add log with level, tag, formatted content
extern int internal_logger_printf(enum log_level level,char*tag,const char*fmt,...) __attribute__((format(printf,3,4)));

// src/loggerd/logger_internal.c: turn on or off logger output
extern int internal_set_logger(char*name,bool enabled);

// src/loggerd/logger_internal.c: set logger output level
extern int internal_set_logger_level(char*name,enum log_level level);

// src/loggerd/logger_internal.c: init a log packaet
extern void internal_init_msg(struct log_msg*msg,enum log_oper oper,size_t size);

// src/loggerd/logger_internal.c: read a log packet
extern int internal_read_msg(int fd,struct log_msg*buff);

// src/loggerd/logger_internal.c: send a log packet
extern int internal_send_msg(int fd,enum log_oper oper,void*data,size_t size);

// src/loggerd/logger_internal.c: read a string log packet
extern int internal_send_msg_string(int fd,enum log_oper oper,char*data);

// src/loggerd/logger_internal.c: convert operation to a readable string
extern char*oper2string(enum log_oper oper);

// src/loggerd/logger_buffer.c: convert log_item to log_buff
extern struct log_buff*internal_item2buff(struct log_item*log);

// src/loggerd/logger_buffer.c: convert log_buff to log_item
extern struct log_item*internal_buff2item(struct log_buff*log);

// src/loggerd/logger_buffer.c: add log to buffer
extern int internal_buffer_push(struct log_item*log);

// src/loggerd/logger_buffer.c: free log_buff
extern int internal_free_buff(void*d);

// src/loggerd/logger_buffer.c: clean log buffers
extern void clean_log_buffers();

// src/loggerd/logger_buffer.c: flush buffer to logger
extern void flush_buffer(struct logger*log);

// src/loggerd/klog.c: read all kmesg to buffer and read new kmsg to logger
extern int init_kmesg();
#endif