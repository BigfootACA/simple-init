#include<stdio.h>
#include<libintl.h>
#include"defines.h"
#include"version.h"

int version_main(int argc,char**argv __attribute__((unused))){
	puts(argc==1?VERSION_INFO:_("Usage: version"));
	return argc-1;
}