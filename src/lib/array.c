#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include"defines.h"
#include"output.h"

char*array2args(char**arr,char*d){
	if(!arr||!d)return NULL;
	char*b,*c;
	size_t s=BUFFER_SIZE*sizeof(char),idx=0,len=0,dc=strlen(d);
	if(!(b=malloc(s)))return NULL;
	memset(b,0,s);
	while((c=arr[idx++])&&len<s-2){
		if(idx>1){
			strcat(b,d);
			len+=dc;
		}
		size_t x=strlen(c);
		if(x+len>=s-1)break;
		len+=x;
		strcat(b,c);
	}
	c=strdup(b);
	free(b);
	return c;
}

char**char_array_append(char**array,char*item,int idx){
	size_t size=sizeof(char*)*(idx+1);
	if(!array){
		if(!(array=malloc(size)))return NULL;
		memset(array,0,size);
	}else if(!(array=realloc(array,size)))return NULL;
	array[idx]=item;
	return array;
}

int char_array_len(char**arr){
	int i=-1;
	if(arr)while(arr[++i]);
	return i;
}

void free_args_array(char**c){
	if(c){
		if(c[0])free(c[0]);
		c[0]=NULL;
		free(c);
	}
	c=NULL;
}

char**args2array(char*source,char del){
	if(!source)return NULL;
	int item=0;
	size_t sz=sizeof(char)*(strlen(source)+1);
	char**array=NULL,*buff=NULL;
	if(!(buff=malloc(sz)))goto fail;
	if(!(array=malloc((sizeof(char*)*(item+2)))))goto fail;
	char*scur=source,*dcur=buff,**arr=NULL,b;
	memset(buff,0,sz);
	array[item]=dcur;
	while(*scur>0)if(del==0?isspace(*scur):del==*scur){
		scur++;
		if(*array[item]){
			dcur++,item++;
			if(!(arr=realloc(array,(sizeof(char*)*(item+2)))))goto fail;
			array=arr;
			array[item]=dcur;
		}
	}else if(*scur=='"'||*scur=='\''){
		b=*scur;
		while(*scur++&&*scur!=b){
			if(*scur=='\\')scur++;
			*dcur++=*scur;
		}
		dcur++,scur++;
	}else *dcur++=*scur++;
	if(!*array[item]){
		array[item]=NULL;
		if(item==0){
			free(buff);
			buff=NULL;
		}
	}
	array[++item]=NULL;
	if(!(arr=malloc((sizeof(char*)*(item+2)))))goto fail;
	memset(arr,0,(sizeof(char*)*(item+2)));
	for(int t=0;array[t];t++)arr[t]=array[t];
	free(array);
	return arr;
	fail:
	if(buff)free(buff);
	if(array)free(array);
	buff=NULL,array=NULL;
	return NULL;
}

char**array_dup(char**orig){
	if(!orig)return NULL;
	size_t s=0;
	while(orig[s++]);
	s*=sizeof(char*);
	char**buff=malloc(s);
	if(!buff)return NULL;
	memset(buff,0,s);
	for(size_t c=0;orig[c];c++)if(!(buff[c]=strdup(orig[c])))goto fail;
	return buff;
	fail:
	for(size_t c=0;buff[c];c++)free(buff[c]);
	free(buff);
	return NULL;
}

void array_free(char**arr){
	if(!arr)return;
	for(size_t c=0;arr[c];c++)free(arr[c]);
	free(arr);
}
