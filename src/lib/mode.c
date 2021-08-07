#define _GNU_SOURCE
#include<sys/stat.h>
// from busybox libbb/mode_string.c
static const mode_t flags[]={
	S_IRUSR,S_IWUSR,S_IXUSR,S_ISUID,
	S_IRGRP,S_IWGRP,S_IXGRP,S_ISGID,
	S_IROTH,S_IWOTH,S_IXOTH,S_ISVTX
};

static const char types[16]="?pc?d?b?-?l?s???";
static const char modes[7]="rwxSTst";

const char*mode_string(mode_t mode){
	static char buf[12];
	char*p=buf;
	int i=0;
	*p=types[(mode>>12)&0xF];
	do{
		int j=0,k=0;
		do{
			*++p='-';
			if(mode&flags[i+j])*p=modes[j],k=j;
		}while(++j<3);
		if(mode&flags[i+j])*p=modes[3+(k&2)+((i&8)>>3)];
		i+=4;
	}while(i<12);
	return buf;
}
