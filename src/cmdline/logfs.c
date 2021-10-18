/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"confd.h"

int cmdline_logfs(char*k __attribute__((unused)),char*v){
	confd_set_string("runtime.cmdline.logfs",v);
	return 0;
}

int cmdline_logfile(char*k __attribute__((unused)),char*v){
	confd_set_string("runtime.cmdline.logfile",v);
	return 0;
}
