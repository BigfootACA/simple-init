// code from musl 1.2.0
#include<string.h>
#include<stdlib.h>
#include"compatible.h"
char*comp_strndup(const char*s,size_t n){
	size_t l=comp_strnlen(s,n);
	char*d=malloc(l+1);
	if(!d)return NULL;
	memcpy(d,s,l);
	d[l]=0;
	return d;
}
