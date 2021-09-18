#ifndef CONFD_H
#define CONFD_H
#include<stdint.h>
#include<stdbool.h>
#include"pathnames.h"
#define DEFAULT_CONFD _PATH_RUN"/confd.sock"

// config data types
enum conf_type{
	TYPE_KEY     =0xAF01,
	TYPE_STRING  =0xAF02,
	TYPE_INTEGER =0xAF03,
	TYPE_BOOLEAN =0xAF04,
};

// src/confd/client.c: open confd socket
extern int open_confd_socket(char*tag,char*path);

// src/confd/client.c: close confd socket
extern void close_confd_socket();

// src/confd/client.c: set confd socket fd
extern int set_confd_socket(int fd);

#ifndef ENABLE_UEFI
// src/confd/client.c: start a config daemon in protect mode
extern int start_confd(char*tag,pid_t*p);
#endif

// src/confd/client.c: terminate remote confd
extern int confd_quit();

// src/confd/client.c: dump config store to logger
extern int confd_dump();

// src/confd/client.c: delete a config item
extern int confd_delete(const char*path);

// src/confd/client.c: set an integer config item
extern int confd_set_integer(const char*path,int64_t def);

// src/confd/client.c: set a string config item
extern int confd_set_string(const char*path,char*def);

// src/confd/client.c: set a boolean config item
extern int confd_set_boolean(const char*path,bool def);

// src/confd/client.c: get type of config item
extern enum conf_type confd_get_type(const char*path);

// src/confd/client.c: get string config item
extern char*confd_get_string(const char*path,char*data);

// src/confd/client.c: get integer config item
extern int64_t confd_get_integer(const char*path,int64_t def);

// src/confd/client.c: get boolean config item
extern bool confd_get_boolean(const char*path,bool def);

// open default socket
#define open_default_confd_socket(tag) open_confd_socket(tag,DEFAULT_CONFD)

#endif
