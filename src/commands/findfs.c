#ifdef ENABLE_BLKID
#include<stdio.h>
#include<blkid/blkid.h>
#include"getopt.h"
#include"output.h"

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: findfs [OPTIONS] {LABEL,UUID,PARTUUID,PARTLABEL}=<VALUE>\n"
		"Find a filesystem by label or UUID.\n"
	);
}

int findfs_main(int argc,char**argv){
	char *dev;
	static const struct option lo[]={
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};
	if(argc!=2)return re_printf(2,"findfs: bad usage\n");
	switch(b_getlopt(argc,argv,"h",lo,0)){
		case -1:break;
		case 'h':return usage(0);
		default:return -1;
	}
	dev=blkid_evaluate_tag(argv[1],NULL,NULL);
	if(!dev)return re_printf(1,"findfs: unable to resolve '%s'\n",argv[1]);
	puts(dev);
	return 0;
}
#endif
