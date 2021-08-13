#include<stdlib.h>
#include<errno.h>
#include<libgen.h>
#include<stdio.h>
#include"proctitle.h"
#include"shell.h"
#include"language.h"
#include"defines.h"
#include"output.h"

int main(int argc,char**argv){
	spt_init(argc,argv);
	lang_init_locale();
	char*name;
	if(
		!(name=getenv("INIT_MAIN"))&&
		!(name=basename(argv[0]))
	)return ee_printf(1,"failed to get name\n");
	int r=invoke_internal_cmd_nofork_by_name(name,argv);
	if(errno!=0)fprintf(stderr,_("%s: command not found\n"),name);
	return r;
}
