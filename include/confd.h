/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef CONFD_H
#define CONFD_H
#include<stdint.h>
#include<stdbool.h>
#include<sys/types.h>
#include"pathnames.h"
#include"logger.h"
#define DEFAULT_CONFD _PATH_RUN"/confd.sock"

// config data types
enum conf_type{
	TYPE_KEY     =0xAF01,
	TYPE_STRING  =0xAF02,
	TYPE_INTEGER =0xAF03,
	TYPE_BOOLEAN =0xAF04,
};

// src/confd/client.c: open confd socket
extern int open_confd_socket(bool quiet,char*tag,char*path);

// src/confd/client.c: check should open confd socket
extern int check_open_confd_socket(bool quiet,char*tag,char*path);

// src/confd/client.c: close confd socket
extern void close_confd_socket(void);

// src/confd/client.c: set confd socket fd
extern int set_confd_socket(int fd);

#ifdef ENABLE_UEFI
// src/confd/uefi.c: initialize config daemon
extern int confd_init();
#else
// src/confd/client.c: start a config daemon in protect mode
extern int start_confd(char*tag,pid_t*p);
#endif

// src/confd/client.c: terminate remote confd
extern int confd_quit(void);

// src/confd/client.c: dump config store to logger
extern int confd_dump(enum log_level level);

// src/confd/client.c: save config store as a file
extern int confd_save_file(const char*file);

// src/confd/client.c: load config store from a file
extern int confd_load_file(const char*file);

// src/confd/client.c: include config store from a file
extern int confd_include_file(const char*file);

// src/confd/client.c: set default config file path
extern int confd_set_default_config(const char*file);

#define DECLARE_FUNC(func,ret,...) \
	extern ret func(const char*path __VA_ARGS__); \
	extern ret func##_base(const char*base,const char*path __VA_ARGS__);\
	extern ret func##_dict(const char*base,const char*key,const char*path __VA_ARGS__);\
	extern ret func##_array(const char*base,int index,const char*path __VA_ARGS__);
DECLARE_FUNC(confd_set_integer, int,     ,int64_t data);
DECLARE_FUNC(confd_set_string,  int,     ,char*   data);
DECLARE_FUNC(confd_set_boolean, int,     ,bool    data);
DECLARE_FUNC(confd_get_sstring, char*,   ,char*   def, char*buf, size_t size);
DECLARE_FUNC(confd_get_string,  char*,   ,char*   data);
DECLARE_FUNC(confd_get_integer, int64_t, ,int64_t data);
DECLARE_FUNC(confd_get_boolean, bool,    ,bool    data);
DECLARE_FUNC(confd_get_own,     int,     ,uid_t*  own);
DECLARE_FUNC(confd_get_grp,     int,     ,gid_t*  grp);
DECLARE_FUNC(confd_get_mod,     int,     ,mode_t* mod);
DECLARE_FUNC(confd_set_own,     int,     ,uid_t   own);
DECLARE_FUNC(confd_set_grp,     int,     ,gid_t   grp);
DECLARE_FUNC(confd_set_mod,     int,     ,mode_t  mod);
DECLARE_FUNC(confd_rename,      int,     ,const char*name);
DECLARE_FUNC(confd_set_save,    int,     ,bool    save);
DECLARE_FUNC(confd_get_save,    bool);
DECLARE_FUNC(confd_delete,      int);
DECLARE_FUNC(confd_add_key,     int);
DECLARE_FUNC(confd_count,       int64_t);
DECLARE_FUNC(confd_ls,          char**);
DECLARE_FUNC(confd_get_type,    enum conf_type);

// open default socket
#define open_default_confd_socket(quiet,tag) open_confd_socket(quiet,tag,DEFAULT_CONFD)
#define check_open_default_confd_socket(quiet,tag) check_open_confd_socket(quiet,tag,DEFAULT_CONFD)

#endif
