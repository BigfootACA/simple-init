/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef TTYD_H
#define TTYD_H
#include<stdint.h>
#include<stdbool.h>
#include<sys/types.h>
#include"pathnames.h"
#define DEFAULT_TTYD _PATH_RUN"/ttyd.sock"

// src/ttyd/client.c: open ttyd socket
extern int open_ttyd_socket(bool quiet,char*tag,char*path);

// src/ttyd/client.c: open ttyd socket
extern int check_open_ttyd_socket(bool quiet,char*tag,char*path);

// src/ttyd/client.c: close ttyd socket
extern void close_ttyd_socket(void);

// src/ttyd/client.c: set ttyd socket fd
extern int set_ttyd_socket(int fd);

// src/ttyd/client.c: terminate remote ttyd
extern int ttyd_quit(void);

// src/ttyd/client.c: call ttyd reload all tty from confd
extern int ttyd_reload(void);

// src/ttyd/client.c: call ttyd reopen all tty
extern int ttyd_reopen(void);

// open default socket
#define open_default_ttyd_socket(quiet,tag) open_ttyd_socket(quiet,tag,DEFAULT_TTYD)
#define check_open_default_ttyd_socket(quiet,tag) check_open_ttyd_socket(quiet,tag,DEFAULT_TTYD)

#endif
