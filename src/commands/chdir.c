#include<errno.h>
#include<unistd.h>
#include"output.h"

int chdir_main(int argc,char**argv){
	if(argc==2&&argv[1])chdir(argv[1]);
	else errno=EINVAL;
	return errno!=0?re_printf(errno,"chdir"):0;
}
