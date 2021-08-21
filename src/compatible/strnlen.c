// code from musl 1.2.0
#include<string.h>
#include"compatible.h"
size_t comp_strnlen(const char*s,size_t n){
	const char*p=memchr(s,0,n);
	return p?p-s:n;
}
