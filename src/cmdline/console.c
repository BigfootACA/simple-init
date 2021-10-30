/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include"confd.h"

int cmdline_console_shell(char*k __attribute__((unused)),char*v __attribute__((unused))){
	confd_set_boolean("runtime.cmdline.console_shell",true);
	return 0;
}
