/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef SHELL_H
#define SHELL_H
#include<stdbool.h>
#include<sys/types.h>

// builtin command main
typedef int cmd_main(int,char**);

// builtin command info
struct shell_command{
	bool enabled,fork;
	char name[32];
	char*help;
	cmd_main*main;
};

#ifdef ENABLE_READLINE

// src/shelld/shell.c: next status code
extern unsigned char exit_code;

// src/shelld/shell.c: run a new shell
extern void run_shell(void);

// src/shelld/shell.c: run_shell wrapper
extern int initshell_main(int argc,char**argv);

// src/shelld/shell.c: register console shell service
extern int register_console_shell(void);

#endif

// src/shelld/cmd.c: find internal command by name
extern struct shell_command*find_internal_cmd(char*name);

// src/shelld/cmd.c: fork and invoke internal command
extern int invoke_internal_cmd(struct shell_command*cmd,bool background,char**args);

// src/shelld/cmd.c: invoke internal command
extern int invoke_internal_cmd_nofork(struct shell_command*cmd,char**args);

// src/shelld/cmd.c: fork and invoke internal command by name
extern int invoke_internal_cmd_by_name(char*name,bool background,char**args);

// src/shelld/cmd.c: invoke internal command by name
extern int invoke_internal_cmd_nofork_by_name(char*name,char**args);

// src/shelld/cmd.c: init commands help i18n
extern int init_commands_locale(void);

// src/shelld/cmd.c: install commands
extern int install_cmds(int dfd);
#endif
