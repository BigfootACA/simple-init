#include<stdio.h>
#include"version.h"

int version_main(int argc,char**argv __attribute__((unused))){
	puts(argc==1?VERSION_INFO:"Usage: version");
	return argc-1;
}