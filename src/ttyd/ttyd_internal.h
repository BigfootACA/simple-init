/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef TTYD_INTERNAL
#define TTYD_INTERNAL
#include<unistd.h>
#include<stdbool.h>
#include<termios.h>
#include<sys/epoll.h>
#include"ttyd.h"
#include"defines.h"
#include"pathnames.h"

#define CTL(x) ((x)^0100)
#define TTYD_MAGIC0 0xEF
#define TTYD_MAGIC1 0x44
enum tty_fd_type{
	FD_TTY,
	FD_SERVER,
	FD_CLIENT,
};
struct tty_data{
	bool init_attr;
	int fd,speed;
	enum tty_fd_type type;
	struct epoll_event ev;
	struct termios attrs;
	char eol,name[64];
	uid_t uid;
	gid_t gid;
	char user[64],group[64];
	char shell[PATH_MAX],home[PATH_MAX];
	pid_t worker;
};
enum ttyd_action{
	TTYD_OK     =0xAA00,
	TTYD_FAIL   =0xAA01,
	TTYD_QUIT   =0xAA02,
	TTYD_RELOAD =0xAA03,
	TTYD_REOPEN =0xAA04,
};
struct ttyd_msg{
	unsigned char magic0:8,magic1:8;
	enum ttyd_action action:16;
	int code:32;
	char data[4088];
};
extern int tty_dev_fd;
extern int tty_epoll_fd;
extern void tty_reopen_all(void);
extern const char*tty_sock;
extern const char*tty_conf_ttys;
extern const char*tty_rt_ttys;
extern const char*tty_rt_tty_clt;
extern void tty_conf_init(void);
extern void tty_conf_add_all(void);
extern void tty_add(const char*base,const char*name);
extern void tty_open(struct tty_data*data);
extern int tty_start_session(struct tty_data*data);
extern int tty_start_worker(struct tty_data*data);
extern int tty_speed_convert(int number);
extern bool tty_exists(const char*name);
extern bool tty_confd_get_boolean(struct tty_data*data,const char*key,bool def);
extern char*tty_confd_get_string(struct tty_data*data,const char*key,char*def);
extern char*tty_issue_replace(char*src,char*dest,size_t dest_len,struct tty_data*data);
extern char*tty_issue_read(char*path,char*dest,size_t dest_len,struct tty_data*data);
extern char*tty_issue_get(char*dest,size_t dest_len,struct tty_data*data);
extern char*tty_read_username(struct tty_data*data);
extern bool tty_ask_pwd(struct tty_data*data,char*username);
extern bool tty_login(struct tty_data*data);
extern ssize_t tty_issue_write(int fd,struct tty_data*data);
extern void ttyd_epoll_client(struct tty_data*data);
extern void ttyd_epoll_server(struct tty_data*data);
extern int ttyd_listen_socket();
extern bool ttyd_internal_check_magic(struct ttyd_msg*msg);
extern void ttyd_internal_init_msg(struct ttyd_msg*msg,enum ttyd_action action);
extern int ttyd_internal_send(int fd,struct ttyd_msg*msg);
extern int ttyd_internal_send_code(int fd,enum ttyd_action action,int code);
extern int ttyd_internal_read_msg(int fd,struct ttyd_msg*buff);
extern const char*ttyd_action2name(enum ttyd_action action);
#endif
