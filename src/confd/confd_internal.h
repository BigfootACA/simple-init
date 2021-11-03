/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef CONFD_INTERNAL_H
#define CONFD_INTERNAL_H
#include<inttypes.h>
#include"str.h"
#include"list.h"
#include"confd.h"

#define CONFD_MAGIC0 0xEF
#define CONFD_MAGIC1 0x66
#define CONF_KEY_CHARS LETTER NUMBER "-_."

// initconfd remote action
enum confd_action{
	CONF_OK           =0xAC00,
	CONF_FAIL         =0xAC01,
	CONF_QUIT         =0xAC02,
	CONF_DELETE       =0xAC03,
	CONF_DUMP         =0xAC04,
	CONF_LIST         =0xAC05,
	CONF_GET_TYPE     =0xAC06,
	CONF_ADD_KEY      =0xAC07,
	CONF_GET_STRING   =0xAC21,
	CONF_GET_INTEGER  =0xAC22,
	CONF_GET_BOOLEAN  =0xAC23,
	CONF_SET_STRING   =0xAC41,
	CONF_SET_INTEGER  =0xAC42,
	CONF_SET_BOOLEAN  =0xAC43,
	CONF_SET_DEFAULT  =0xACA1,
	CONF_SAVE         =0xACA2,
	CONF_LOAD         =0xACA3,
	CONF_SET_SAVE     =0xACA4,
	CONF_GET_SAVE     =0xACA5,
};

// initconfd message
struct confd_msg{
	unsigned char magic0:8,magic1:8;
	enum confd_action action:16;
	char path[4084-(MAX(
		MAX(sizeof(size_t),sizeof(int64_t)),
		MAX(sizeof(enum conf_type),sizeof(bool))
	))];
	int code:32;
	union{
		size_t data_len;
		enum conf_type type;
		int64_t integer;
		bool boolean;
	}data;
};

// config struct
struct conf{
	char name[255];
	enum conf_type type;
	struct conf*parent;
	uid_t user;
	gid_t group;
	mode_t mode;
	bool save;
	union{
		list*keys;
		union{
			char*string;
			int64_t integer;
			bool boolean;
		}value;
	};
};

#ifdef ENABLE_UEFI
#include<Protocol/SimpleFileSystem.h>
#define _ROOT_TYPE EFI_FILE_PROTOCOL*
#else
#define _ROOT_TYPE int
#endif

typedef int(*conf_file_hand_func)(_ROOT_TYPE root,const char*path);

// config file handler
struct conf_file_hand{
	char**ext;
	conf_file_hand_func load;
	conf_file_hand_func save;
};

extern bool conf_store_changed;

extern struct conf_file_hand*conf_hands[];

// src/confd/client.c: current confd fd
extern int confd;

// src/confd/internal.c: check message magick
extern bool confd_internal_check_magic(struct confd_msg*msg);

// src/confd/internal.c: check message magick
extern void confd_internal_init_msg(struct confd_msg*msg,enum confd_action action);

// src/confd/internal.c: send message
extern int confd_internal_send(int fd,struct confd_msg*msg);

// src/confd/internal.c: send code
extern int confd_internal_send_code(int fd,enum confd_action action,int code);

// src/confd/internal.c: read message
extern int confd_internal_read_msg(int fd,struct confd_msg*buff);

// src/confd/internal.c: convert action to string
extern const char*confd_action2name(enum confd_action action);

// src/confd/dump.c: dump config store to loggerd
extern int conf_dump_store();

// src/confd/store.c: get config store root struct
extern struct conf*conf_get_store();

// src/confd/store.c: convert config item type to string
extern const char*conf_type2string(enum conf_type type);

// src/confd/store.c: get config item type by path
extern enum conf_type conf_get_type(const char*path,uid_t u,gid_t g);

// src/confd/store.c: get config item type string by path
extern const char*conf_get_type_string(const char*pat,uid_t u,gid_t gh);

// src/confd/store.c: list config item keys
extern const char**conf_ls(const char*path,uid_t u,gid_t g);

// src/confd/store.c: delete config item and all children
extern int conf_del(const char*path,uid_t u,gid_t g);

// src/confd/store.c: create a config key
extern int conf_add_key(const char*path,uid_t u,gid_t g);

// src/confd/store.c: set config should save
extern int conf_set_save(const char*path,bool save,uid_t u,gid_t g);

// src/confd/store.c: get config should save
extern bool conf_get_save(const char*path,uid_t u,gid_t g);

// src/confd/file.c: load config file to config store
extern int conf_load_file(_ROOT_TYPE root,const char*path);

// src/confd/file.c: save config store to config file
extern int conf_save_file(_ROOT_TYPE root,const char*path);

// config item value
#define VALUE_STRING(conf)conf->value.string
#define VALUE_BOOLEAN(conf)conf->value.boolean
#define VALUE_INTEGER(conf)conf->value.integer

// src/confd/store.c: get or set config item value
#define DECLARE_CONF_GET_SET(_tag,_type,_func) \
	extern int conf_set_##_func(const char*path,_type data,uid_t u,gid_t g);\
	extern _type conf_get_##_func(const char*path,_type def,uid_t u,gid_t g);

DECLARE_CONF_GET_SET(STRING,char*,string)
DECLARE_CONF_GET_SET(INTEGER,int64_t,integer)
DECLARE_CONF_GET_SET(BOOLEAN,bool,boolean)

#endif
