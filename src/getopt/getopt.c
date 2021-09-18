// file from musl 1.2.0, override libc
#define _GNU_SOURCE
#include<errno.h>
#include<wchar.h>
#include<limits.h>
#include<stdlib.h>
#include<stdio.h>
#include"defines.h"
#include"getopt.h"
char *b_optarg;
int b_optind=1;
int b_opterr=1;
int b_optopt;
int __b_optpos;
int __b_optreset=0;

int b_getopt(int argc,char*const*argv,const char*optstring){
	int i,k,l;
	wchar_t c,d;
	char*optchar;
	if(!optstring)return -1;
	if(!b_optind||__b_optreset)
		__b_optreset=0,__b_optpos=0,b_optind=1;
	if(b_optind>=argc||!argv[b_optind])return -1;
	if(argv[b_optind][0]!='-'){
		if(optstring[0]=='-'){
			b_optarg=argv[b_optind++];
			return 1;
		}
		return -1;
	}
	if(!argv[b_optind][1])return -1;
	if(argv[b_optind][1]=='-'&&!argv[b_optind][2])return b_optind++,-1;
	if(!__b_optpos)__b_optpos++;
	if((k=mbtowc(&c,argv[b_optind]+__b_optpos,MB_LEN_MAX))<0)k=1,c=0xfffd;
	optchar=argv[b_optind]+__b_optpos,__b_optpos+=k;
	if(!argv[b_optind][__b_optpos])b_optind++,__b_optpos=0;
	if(optstring[0]=='-'||optstring[0]=='+')optstring++;
	i=0,d=0;
	do{l=mbtowc(&d,optstring+i,MB_LEN_MAX),i+=l>0?l:1;}while(l&&d!=c);
	if(d!=c||c==':'){
		b_optopt=c;
		if(optstring[0]!=':'&&b_opterr)fprintf(
			stderr,
			_("%s: unrecognized option: %s\n"),
			program_invocation_short_name,
			optchar
		);
		return '?';
	}
	if(optstring[i]==':'){
		b_optarg=0;
		if(optstring[i+1]!=':'||__b_optpos){
			b_optarg=
				argv[b_optind++]+
				__b_optpos;
			__b_optpos=0;
		}
		if(b_optind>argc){
			b_optopt=c;
			if(optstring[0]==':')return ':';
			if(b_opterr)fprintf(
				stderr,
				_("%s: option requires an argument: %s\n"),
				program_invocation_short_name,
				optchar
			);
			return '?';
		}
	}
	return c;
}
