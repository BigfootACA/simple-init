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
#include"defines.h"

#define CTL(x) ((x)^0100)
struct tty_data{
	bool init_attr;
	int fd,speed;
	struct epoll_event ev;
	struct termios attrs;
	char eol,name[64];
	uid_t uid;
	gid_t gid;
	char user[64],group[64];
	char shell[PATH_MAX],home[PATH_MAX];
	pid_t worker;
};
extern int tty_dev_fd;
extern int tty_epoll_fd;
extern void tty_reopen_all();
extern const char*tty_conf_ttys;
extern const char*tty_rt_ttys;
extern const char*tty_rt_tty_clt;
extern void tty_conf_init();
extern void tty_conf_add_all();
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
#endif
