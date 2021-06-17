#ifndef INIT_INTERNAL_H
#define INIT_INTERNAL_H
#include<sys/socket.h>
#define DEFAULT_INITD _PATH_RUN"/initd.sock"

// initd packet magic
#define INITD_MAGIC0 0xEF
#define INITD_MAGIC1 0x99

enum init_status{
	INIT_RUNNING      =0xCE01,
	INIT_BOOT         =0xCE02,
	INIT_SHUTDOWN     =0xCE03,
};
extern enum init_status status;

enum init_action{
	ACTION_NONE       =0xBE00,
	ACTION_OK         =0xBE01,
	ACTION_FAIL       =0xBE02,
	ACTION_POWEROFF   =0xBE03,
	ACTION_HALT       =0xBE04,
	ACTION_REBOOT     =0xBE05,
	ACTION_SWITCHROOT =0xBE06,
};
extern enum init_action action;

union action_data{
	int ret;
	char data[1016];
	struct{
		char root[508];
		char init[508];
	}newroot;
};
extern union action_data actiondata;

struct init_msg{
	unsigned char magic0,magic1;
	enum init_action action;
	union action_data data;
};

#include"init.h"

#ifdef _GNU_SOURCE
// src/initd/protocol.c: process init_msg
extern int init_process_data(int cfd,struct ucred*u,struct init_msg*msg);

// src/initd/protocol.c: check ucred permission
extern bool init_check_privilege(enum init_action act,struct ucred*cred);
#endif

// src/initd/protocol.c: check init_msg validity
extern bool init_check_msg(struct init_msg*msg);

// src/initd/protocol.c: initialize init_msg
extern void init_initialize_msg(struct init_msg*msg,enum init_action act);

// src/initd/protocol.c: send init_msg
extern int init_send_data(int fd,struct init_msg*send);

// src/initd/protocol.c: receive init_msg
extern int init_recv_data(int fd,struct init_msg*response);

// src/initd/protocol.c: convert init_action to string
extern const char*action2string(enum init_action action);

// src/initd/socket.c: listen init socket
extern int listen_init_socket();

// src/initd/socket.c: process socket epoll
extern int init_process_socket(int sfd);

#endif
