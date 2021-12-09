/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef SHELL_INTERNAL_H
#define SHELL_INTERNAL_H
#include<stdbool.h>
#include<stddef.h>
#include"shell.h"
#define TAG "initshell"

// src/shelld/shell.c: should shell keep running
extern bool shell_running;

// src/shelld/commands.c: shell builtin commands list
extern const struct shell_command*shell_cmds[];

// src/shelld/cmd.c: execute command with args
extern int run_cmd(char**args,bool background);

// src/shelld/external.c: execute external command with args
extern int run_external_cmd(char**argv,bool background);

// src/shelld/replace.c: generate shell prompt string
extern char*shell_replace(char*dest,char*src,size_t size);

// declare builtin command main
#define DECLARE_MAIN(name) extern int name##_main(int,char**)

// declare builtin command info
#define DECLARE_CMD(fork,name,help) &((struct shell_command){true,fork,#name,help,&name##_main}),
#define DECLARE_CMD_MAIN(fork,name,help,main) &((struct shell_command){true,fork,#name,help,&main}),
#endif
