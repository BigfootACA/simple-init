// code from musl 1.2.0
#include<string.h>
#include<stdlib.h>
#include"compatible.h"
char*comp_strdup(const char*s){
        size_t l=strlen(s);
        char*d=malloc(l+1);
        if(!d)return NULL;
        return memcpy(d,s,l+1);
}