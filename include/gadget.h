/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _GADGET_H
#define _GADGET_H
#include"keyval.h"
#define wr_file(fd,file,str)write_file(fd,file,str,strlen(str),0644,false,true,true)

// gadget config string (/sys/kernel/config/usb_gadget/XXX/configs/XXX.X/strings/0xXXX/configuration)
struct gadget_config_string{
	int id;
	char*configuration;
};
typedef struct gadget_config_string gadget_cfgstr;

// gadget string (/sys/kernel/config/usb_gadget/XXX/strings/0xXXX/)
struct gadget_string{
	int id;
	char
		*manufacturer,
		*product,
		*serialnumber;
};
typedef struct gadget_string gadget_str;

// gadget config (/sys/kernel/config/usb_gadget/XXX/configs/XXX.X/)
struct gadget_config{
	char type[16];
	int
		id,
		attributes,
		max_power;
	gadget_cfgstr**strings;
};
typedef struct gadget_config gadget_cfg;

// gadget function (/sys/kernel/config/usb_gadget/XXX/functions/XXXX.XX/)
struct gadget_function{
	char*function;
	char*name;
	keyval**values;
};
typedef struct gadget_function gadget_func;

// gadget generic info (/sys/kernel/config/usb_gadget/XXX/)
struct gadget{
	char*name;
	int
		dir_fd,
		product,
		vendor,
		device,
		USB,
		devProtocol,
		devClass,
		devSubClass;
	gadget_cfg**configs;
	gadget_str**strings;
};
typedef struct gadget gadget;

// declare gadget string
#define GADGET_STR(id,man,pro,ser) &(gadget_str){id,man,pro,ser}

// declare gadget config string
#define GADGET_CFGSTR(id,val) &(gadget_cfgstr){id,val}

// declare gadget config array type
#define GADGET_CFGARRAY (gadget_cfg*[])

// declare gadget string array type
#define GADGET_STRARRAY (gadget_str*[])

// declare gadget config config array type
#define GADGET_CFGSTRARRAY (gadget_cfgstr*[])

// src/gadget/register.c: auto init usb_gadget and register a gadget
extern int gadget_register(gadget*gadget);

// src/gadget/register.c: register a gadget with specified gadget folder fd
extern int gadget_register_fd(int sfs,gadget*gadget);

// src/gadget/unregister.c: auto init usb_gadget and unregister a gadget
extern int gadget_unregister(char*gadget_name);

// src/gadget/unregister.c: unregister a gadget with specified gadget folder fd
extern int gadget_unregister_fd(int dfd,char*gadget_name);

// src/gadget/unregister.c: auto init usb_gadget and unregister all gadgets
extern int gadget_unregister_all(void);

// src/gadget/unregister.c: unregister all gadgets with specified gadget folder fd
extern int gadget_unregister_all_fd(int dfd);

// src/gadget/add_function.c: add a function to gadget and auto select a config
extern int gadget_add_function(gadget*gadget,gadget_func*func);

// src/gadget/add_function.c: add a function to gadget
extern int gadget_add_function_cfg(gadget*gadget,gadget_cfg*cfg,gadget_func*func);

// src/gadget/add.c: add a config to gadget
extern int gadget_add_config(gadget*gadget,gadget_cfg*cfg);

// src/gadget/add.c: add a gadget string
extern int gadget_add_string(gadget*gadget,gadget_str*str);

// src/gadget/general.c: write gadget info to sysfs
extern int gadget_write_info(gadget*gadget);

// src/gadget/startstop.c: stop a gadget
extern int gadget_stop(gadget*gadget);

// src/gadget/startstop.c: start a gadget
extern int gadget_start(gadget*gadget,const char*udc);

// src/gadget/general.c: search an available usb gadget controller
extern char*gadget_find_udc(void);

// src/gadget/general.c: set udc
extern int gadget_write_udc(int fd,const char*udc);

// src/gadget/register.c: register gadget by specified path
extern int gadget_register_path(char*path,gadget*g);

// src/gadget/startstop.c: remove gadget udc
extern int gadget_stop_fd(int fd);

// src/gadget/startstop.c: write gadget udc
extern int gadget_start_fd(int fd,const char*device);

// src/gadget/startstop.c: find and write gadget udc
extern int gadget_start_auto_fd(int fd);

// src/gadget/startstop.c: find udc and start gadget
extern int gadget_start_auto(gadget*g);

// src/gadget/add_function.c: alias of gadget_add_function(gadget,{function,name,values})
extern int gadget_add_function_var(
	gadget*gadget,
	char*function,
	char*name,
	keyval**values
);

// src/gadget/add_function.c: alias of gadget_add_function_cfg(gadget,config,{function,name,values})
extern int gadget_add_function_var_cfg(
	gadget*gadget,
	char*function,
	char*name,
	gadget_cfg*config,
	keyval**values
);

// src/gadget/general.c: init usb_gadget and open
extern int open_usb_gadget(void);

// src/gadget/general.c: register gadget service
extern int register_gadget_service(void);

#endif
