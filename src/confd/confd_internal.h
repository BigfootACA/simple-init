#ifndef CONFD_INTERNAL_H
#define CONFD_INTERNAL_H
#include<inttypes.h>
#include"list.h"

// config data types
enum conf_type{
	TYPE_KEY     =0xAF01,
	TYPE_STRING  =0xAF02,
	TYPE_INTEGER =0xAF03,
	TYPE_BOOLEAN =0xAF04,
};

// config struct
struct conf{
	char name[255];
	enum conf_type type;
	struct conf*parent;
	union{
		list*keys;
		union{
			char*string;
			int64_t integer;
			bool boolean;
		}value;
	};
};

// src/confd/dump.c: dump config store to loggerd
extern int conf_dump_store();

// src/confd/store.c: get config store root struct
extern struct conf*conf_get_store();

// src/confd/store.c: convert config item type to string
extern const char*conf_type2string(enum conf_type type);

// src/confd/store.c: get config item type by path
extern enum conf_type conf_get_type(const char*path);

// src/confd/store.c: get config item type string by path
extern const char*conf_get_type_string(const char*path);

// src/confd/store.c: list config item keys
extern const char**conf_ls(const char*path);

// src/confd/store.c: delete config item and all children
int conf_del(const char*path);

// config item value
#define VALUE_STRING(conf)conf->value.string
#define VALUE_BOOLEAN(conf)conf->value.boolean
#define VALUE_INTEGER(conf)conf->value.integer

// src/confd/store.c: get or set config item value
#define DECLARE_CONF_GET_SET(_tag,_type,_func) \
	extern int conf_set_##_func(const char*path,_type data);\
	extern _type conf_get_##_func(const char*path,_type def);

DECLARE_CONF_GET_SET(STRING,char*,string)
DECLARE_CONF_GET_SET(INTEGER,int64_t,integer)
DECLARE_CONF_GET_SET(BOOLEAN,bool,boolean)

#endif
