#define _GNU_SOURCE
#include<stdio.h>

const char*make_readable_str(unsigned long long val,unsigned long block_size,unsigned long display){
	static const char units[]="\0KMGTPEZY";
	unsigned frac=0;
	const char*u=units,*fmt="%llu";
	if(val==0)return"0";
	if(block_size>1)val*=block_size;
	if(display)val+=display/2,val/=display;
	else{
		while((val>=1024))fmt="%llu.%u%c",u++,frac=(((unsigned)val%1024)*10+1024/2)/1024,val/=1024;
		if(frac>=10)++val,frac=0;
		if(block_size==0){
			if(frac>=5)++val;
			fmt="%llu%*c",frac=1;
		}
	}
	char*ptr;
	return asprintf(&ptr,fmt,val,frac,*u)<0?NULL:ptr;
}