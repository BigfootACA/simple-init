#include<stdio.h>
#include<sys/utsname.h>
#include"output.h"

int arch_main(int argc,char**argv __attribute__((unused))){
	if(argc!=1)return re_printf(1,"Usage: arch\n");
	struct utsname uts;
	uname(&uts);
	puts(uts.machine);
	return 0;
}