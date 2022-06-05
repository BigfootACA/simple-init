/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stdlib.h>
#include<errno.h>
#include<libgen.h>
#include<stdio.h>
#include"proctitle.h"
#include"shell.h"
#include"confd.h"
#include"language.h"
#include"defines.h"
#include"output.h"

extern void _mxml_global(void);

// simple-init linux entry point
int _simple_init_primary_main(int argc,char**argv){

	// init proctitle
	spt_init(argc,argv);

	// workaround for mxml pthread_key_delete crash (?)
	_mxml_global();

	// init config daemon connection
	open_default_confd_socket(true,NULL);

	// init i18n locales
	lang_init_locale();
	char*name;
	if(
		// get command name from environment variable
		!(name=getenv("INIT_MAIN"))&&

		// get command name from argv 0
		!(name=basename(argv[0]))
	)return ee_printf(1,"failed to get name\n");

	// execute command
	int r=invoke_internal_cmd_nofork_by_name(name,argv);

	// command not found
	if(errno!=0)fprintf(stderr,_("%s: command not found\n"),name);
	return r;
}
