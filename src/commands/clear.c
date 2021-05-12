#include<stdio.h>
#include"output.h"

int clear_main(int argc,char**argv __attribute__((unused))){
	if(argc!=1)return re_printf(1,"Usage: clear\n");
	printf("\033[H\033[2J\033[3J");
	return 0;
}