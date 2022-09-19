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
#include"lock.h"
#include"list.h"
#include"confd.h"
#include"filesystem.h"

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
	CONF_COUNT        =0xAC08,
	CONF_RENAME       =0xAC09,
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
	CONF_INCLUDE      =0xACA6,
	CONF_SET_OWNER    =0xACB1,
	CONF_GET_OWNER    =0xACB2,
	CONF_SET_GROUP    =0xACB3,
	CONF_GET_GROUP    =0xACB4,
	CONF_SET_MODE     =0xACB5,
	CONF_GET_MODE     =0xACB6,
};

// initconfd message
struct confd_msg{
	unsigned char magic0,magic1;
	enum confd_action action;
	char path[4096-(sizeof(void*)*3)];
	uint64_t code;
	union{
		size_t data_len;
		enum conf_type type;
		int64_t integer;
		bool boolean;
		uid_t uid;
		gid_t gid;
		mode_t mode;
	}data;
};

// config struct
struct conf{
	char name[256];
	struct conf*parent;
	enum conf_type type;
	uid_t user;
	gid_t group;
	mode_t mode;
	bool save;
	bool include;
	union{
		list*keys;
		union{
			char*string;
			int64_t integer;
			bool boolean;
		}value;
	};
};

struct conf_file_hand;
typedef int(*file_process_func)(struct conf_file_hand*hand);
typedef ssize_t(*file_io_func)(struct conf_file_hand*hand,char*buff,size_t len);

// config file handler
struct conf_file_hand{
	char**ext;
	mutex_t lock;
	bool initialized;
	bool include;
	int depth;
	char*path;
	char*buff;
	size_t len;
	size_t off;
	fsh*parent;
	fsh*file;
	file_process_func load;
	file_process_func save;
	file_io_func read;
	file_io_func write;
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
extern int conf_dump_store(enum log_level level);

// src/confd/store.c: get config store root struct
extern struct conf*conf_get_store(void);

// src/confd/store.c: convert config item type to string
extern const char*conf_type2string(enum conf_type type);

// src/confd/store.c: get config item type by path
extern enum conf_type conf_get_type(const char*path,uid_t u,gid_t g);

// src/confd/store.c: get config item type string by path
extern const char*conf_get_type_string(const char*pat,uid_t u,gid_t gh);

// src/confd/store.c: list config item keys
extern const char**conf_ls(const char*path,uid_t u,gid_t g);

// src/confd/store.c: get config item keys count
extern int conf_count(const char*path,uid_t u,gid_t g);

// src/confd/store.c: delete config item and all children
extern int conf_del(const char*path,uid_t u,gid_t g);

// src/confd/store.c: rename config item
extern int conf_rename(const char*path,const char*name,uid_t u,gid_t g);

// src/confd/store.c: create a config key
extern int conf_add_key(const char*path,uid_t u,gid_t g);

// src/confd/store.c: set config should save
extern int conf_set_save(const char*path,bool save,uid_t u,gid_t g);

// src/confd/store.c: get config should save
extern bool conf_get_save(const char*path,uid_t u,gid_t g);

// src/confd/store.c: get config owner
extern int conf_get_own(const char*path,uid_t*own,uid_t u,gid_t g);

// src/confd/store.c: get config group
extern int conf_get_grp(const char*path,gid_t*grp,uid_t u,gid_t g);

// src/confd/store.c: get config mode
extern int conf_get_mod(const char*path,mode_t*mod,uid_t u,gid_t g);

// src/confd/store.c: set config owner
extern int conf_set_own(const char*path,uid_t own,uid_t u,gid_t g);

// src/confd/store.c: set config group
extern int conf_set_grp(const char*path,gid_t grp,uid_t u,gid_t g);

// src/confd/store.c: set config mode
extern int conf_set_mod(const char*path,mode_t mod,uid_t u,gid_t g);

// src/confd/store.c: calculate conf store memory size
extern size_t conf_calc_size(struct conf*c);

// src/confd/file.c: load config file to config store
extern int conf_load_file(fsh*parent,const char*path);

// src/confd/file.c: include config file to config store
extern int conf_include_file(fsh*parent,const char*path);

// src/confd/file.c: include config file to config store with depth
extern int conf_include_file_depth(fsh*parent,const char*file,int depth);

// src/confd/file.c: save config store to config file
extern int conf_save_file(fsh*parent,const char*path);

// src/confd/file.c: get all supported config file exts
extern char**conf_get_supported_exts();

// config item value
#define VALUE_STRING(conf)conf->value.string
#define VALUE_BOOLEAN(conf)conf->value.boolean
#define VALUE_INTEGER(conf)conf->value.integer

// src/confd/store.c: get or set config item value
#define DECLARE_CONF_GET_SET(_tag,_type,_func) \
	extern int conf_set_##_func(const char*path,_type data,uid_t u,gid_t g);\
	extern int conf_set_##_func##_inc(const char*path,_type data,uid_t u,gid_t g,bool inc);\
	extern _type conf_get_##_func(const char*path,_type def,uid_t u,gid_t g);

DECLARE_CONF_GET_SET(STRING,char*,string)
DECLARE_CONF_GET_SET(INTEGER,int64_t,integer)
DECLARE_CONF_GET_SET(BOOLEAN,bool,boolean)

#endif
