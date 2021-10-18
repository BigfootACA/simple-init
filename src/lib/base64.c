/*
 *
 * Copyright 2008, The Android Open Source Project
 *
 */

#define _GNU_SOURCE
#include<ctype.h>
#include<string.h>
#include<sys/types.h>
#include"str.h"
static const char base64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char pad64='=';
int b64_pton(char const*src,u_char*target,size_t targsize){
	size_t tarindex;
	int state,ch;
	char*pos;
	if(!src||!target)return -1;
	state=0;
	tarindex=0;
	while((ch=(u_char)*src++)!='\0'){
		if(isspace(ch))continue;
		if(ch==pad64)break;
		if((pos=strchr(base64,ch))==0)return(-1);
		switch(state){
			case 0:
				if(tarindex>=targsize)return(-1);
				target[tarindex]=(pos-base64)<<2;
				state=1;
			break;
			case 1:
				if(tarindex+1>=targsize)return(-1);
				target[tarindex]|=(u_int32_t)(pos-base64)>>4;
				target[tarindex+1]=((pos-base64)&0x0f)<<4;
				tarindex++;
				state=2;
			break;
			case 2:
				if(tarindex+1>=targsize)return(-1);
				target[tarindex]|=(u_int32_t)(pos-base64)>>2;
				target[tarindex+1]=((pos-base64)&0x03)<<6;
				tarindex++;
				state=3;
			break;
			case 3:
				if(tarindex>=targsize)return(-1);
				target[tarindex]|=(pos-base64);
				tarindex++;
				state=0;
			break;
		}
	}
	if(ch==pad64){
		ch=*src++;
		switch(state){
			case 0:case 1:return(-1);
			case 2:
				for(;ch!='\0';ch=(u_char)*src++)if(!isspace(ch))break;
				if(ch!=pad64)return(-1);
				ch=*src++;
			// FALLTHROUGH
			case 3:
				for(;ch!='\0';ch=(u_char)*src++)if(!isspace(ch))return(-1);
				if(target[tarindex]!=0)return(-1);
			break;
		}
	}else if(state!=0)return(-1);
	return(tarindex);
}
