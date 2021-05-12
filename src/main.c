#include<stdlib.h>
#include<errno.h>
#include<libgen.h>
#include<stdio.h>
#include"shell.h"

int main(int argc __attribute__((unused)),char**argv){
	struct shell_command*cmd;
	char*name,**a;
	if((name=argv[1])){
		a=argv+1;
		if((cmd=find_internal_cmd(name)))goto ret;
	}
	a=argv;
	if(
		((name=getenv("INIT_MAIN"))||
		(name=basename(argv[0])))&&
		(cmd=find_internal_cmd(name)
	))goto ret;
	fprintf(stderr,"%s: command not found\n",name);
	return ENOENT;
	ret:return invoke_internal_cmd_nofork(cmd,a);
}
