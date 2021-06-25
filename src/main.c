#include<stdlib.h>
#include<errno.h>
#include<libgen.h>
#include<stdio.h>
#include<locale.h>
#include<libintl.h>
#include"proctitle.h"
#include"shell.h"
#include"version.h"
#include"defines.h"
#include"pathnames.h"
#include"output.h"

int main(int argc,char**argv){
	spt_init(argc,argv);
	setlocale(LC_ALL,"");
	bindtextdomain(NAME,_PATH_USR"/share/po");
	textdomain("simple-init");
	init_commands_locale();
	char*name;
	if(
		!(name=getenv("INIT_MAIN"))&&
		!(name=basename(argv[0]))
	)return ee_printf(1,"failed to get name\n");
	int r=invoke_internal_cmd_nofork_by_name(name,argv);
	if(errno!=0)fprintf(stderr,_("%s: command not found\n"),name);
	return r;
}
