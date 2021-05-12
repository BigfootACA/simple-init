#define _GNU_SOURCE
#include<time.h>
#include<errno.h>
#include<stdio.h>
#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<unistd.h>
#include"str.h"

char*time2nstr(time_t*time,char*format,char*buff,size_t len){
	struct tm*timeinfo;
	timeinfo=localtime(time);
	strftime(buff,len,format,timeinfo);
	return buff;
}

char*time2str(time_t*time,char*format,char*buff){
	return time2nstr(time,format,buff,sizeof(buff));
}

char*time2defstr(time_t*time,char*buff){
	return time2str(time,_DEFAULT_TIME_FORMAT,buff);
}

char*time2ndefstr(time_t*time,char*buff,size_t len){
	return time2nstr(time,_DEFAULT_TIME_FORMAT,buff,len);
}

char*new_string(size_t size){
	char*a=malloc(size);
	if(!a)return NULL;
	memset(a,0,size);
	return a;
}

bool contains_of(const char*source,size_t size,char x){
	size_t idx=0;
	char c;
	if(!source)return false;
	while(idx<size&&(c=source[idx++])>0)if(c==x)return true;
	return false;
}

bool check_nvalid(const char*source,size_t size_source,const char*valid,size_t size_valid){
	size_t idx=0;
	char c;
	if(!source||!valid)return false;
	while(idx<size_source&&(c=source[idx++])>0)if(!contains_of(valid,size_valid,c))return false;
	return true;
}

bool check_valid(char*source,const char*valid){
	return check_nvalid(source,strlen(source),valid,strlen(valid));
}

bool check_nvalid_default(char*source,size_t size){
	return check_nvalid(
		source,
		size,
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_",
		63
	);
}

bool check_valid_default(char*source){
	return check_nvalid_default(source,strlen(source));
}

char dec2hex(int dec,bool upper){
	if(dec>=0&&dec<=9)return (char)('0'+dec);
	else if(dec>=10&&dec<=15)return (char)((upper?'A':'a')+dec-10);
	else return 0;
}

char*gen_rand_hex(char*buff,int size,bool upper){
	if(size<0||!buff)return NULL;
	int idx=0;
	srand((unsigned)time(NULL)/size);
	while(idx<size)buff[idx++]=dec2hex(rand()%16,upper);
	buff[idx]=0;
	return buff;
}

char*strrep(char*str,char from,char to){
	size_t s=0;
	while(str[s]&&str[s]!=from)s++;
	if(str[s]==from)str[s]=to;
	return str;
}

int repeat(int fd,char c,size_t times){
	char*buff=malloc((times+1)*sizeof(char));
	if(!buff)return -errno;
	memset(buff,c,times);
	buff[times]=0;
	int r=dprintf(fd,"%s",buff);
	free(buff);
	fsync(fd);
	return r;
}

long parse_long(char*str,long def){
	if(!str)return def;
	errno=0;
	char*end;
	long val=strtol(str,&end,0);
	return errno!=0||end==str?def:val;
}

int parse_int(char*str,int def){
	return (int)parse_long(str,(int)def);
}
