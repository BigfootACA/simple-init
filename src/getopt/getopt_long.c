// file from musl 1.2.0, override libc
#define _GNU_SOURCE
#include<errno.h>
#include<stdlib.h>
#include<limits.h>
#include<stdio.h>
#include<string.h>
#include"defines.h"
#include"getopt.h"

extern int __b_optpos,__b_optreset;

static void b_permute(char*const*argv,int dest,int src){
	char**av=(char**)argv,*tmp=av[src];
	int i;
	for(i=src;i>dest;i--)av[i]=av[i-1];
	av[dest]=tmp;
}

static int __b_getlopt_core(
	int argc,
	char*const*argv,
	const char*optstring,
	const struct option*longopts,
	int*idx,
	int longonly
){
	b_optarg=0;
	if(
		longopts&&
		argv[b_optind][0]=='-'&&
		((
			 longonly&&
			 argv[b_optind][1]&&
			 argv[b_optind][1]!='-'
		)||(
			 argv[b_optind][1]=='-'&&
			 argv[b_optind][2]
		))
	){
		int colon=optstring[optstring[0]=='+'||optstring[0]=='-']==':';
		int i,cnt,match=0;
		char*arg=NULL,*opt,*start=argv[b_optind]+1;
		for(cnt=i=0;longopts[i].name;i++){
			const char*name=longopts[i].name;
			opt=start;
			if(*opt=='-')opt++;
			while(*opt&&*opt!='='&&*opt==*name)name++,opt++;
			if(*opt&&*opt!='=')continue;
			arg=opt,match=i;
			if(!*name){cnt=1;break;}
			cnt++;
		}
		if(cnt==1&&longonly&&arg-start==mblen(start,MB_LEN_MAX)){
			int l=(int)(arg-start);
			for(i=0;optstring[i];i++){
				int j;
				for(j=0;j<l&&start[j]==optstring[i+j];j++);
				if(j==l){cnt++;break;}
			}
		}
		if(cnt==1){
			i=match,opt=arg,b_optind++;
			if(*opt=='='){
				if(!longopts[i].has_arg){
					b_optopt=longopts[i].val;
					if(colon||!b_opterr)return '?';
					fprintf(
						stderr,
						_("%s: option does not take an argument: %s\n"),
						program_invocation_short_name,
						longopts[i].name
					);
					return '?';
				}
				b_optarg=opt+1;
			}else if(longopts[i].has_arg==required_argument){
				if(!(b_optarg=argv[b_optind])){
					b_optopt=longopts[i].val;
					if(colon)return ':';
					if(!b_opterr)return '?';
					fprintf(
						stderr,
						_("%s: option requires an argument: %s\n"),
						program_invocation_short_name,
						longopts[i].name
					);
					return '?';
				}
				b_optind++;
			}
			if(idx)*idx=i;
			if(longopts[i].flag){
				*longopts[i].flag=longopts[i].val;
				return 0;
			}
			return longopts[i].val;
		}
		if(argv[b_optind][1]=='-'){
			b_optopt=0;
			if(!colon&&b_opterr)fprintf(
				stderr,
				cnt?
					_("%s: option is ambiguous: %s\n"):
					_("%s: unrecognized option: %s\n"),
				program_invocation_short_name,
				argv[b_optind]+2
			);
			b_optind++;
			return '?';
		}
	}
	return b_getopt(argc,argv,optstring);
}

static int __b_getlopt(
	int argc,
	char*const*argv,
	const char*optstring,
	const struct option*longopts,
	int*idx,
	int longonly
){
	int ret,skipped,resumed;
	if(!b_optind||__b_optreset)
		__b_optreset=0,__b_optpos=0,b_optind=1;
	if(b_optind>=argc||!argv[b_optind])return -1;
	skipped=b_optind;
	if(optstring[0]!='+'&&optstring[0]!='-'){
		int i;
		for(i=b_optind;;i++){
			if(i>=argc||!argv[i])return -1;
			if(argv[i][0]=='-'&&argv[i][1])break;
		}
		b_optind=i;
	}
	resumed=b_optind;
	ret=__b_getlopt_core(argc,argv,optstring,longopts,idx,longonly);
	if(resumed>skipped){
		int i,cnt=b_optind-resumed;
		for(i=0;i<cnt;i++)b_permute(argv,skipped,b_optind-1);
		b_optind=skipped+cnt;
	}
	return ret;
}

int b_getlopt(
	int argc,
	char*const*argv,
	const char*optstring,
	const struct option*longopts,
	int*idx
){
	return __b_getlopt(argc,argv,optstring,longopts,idx,0);
}

int b_getlopt_only(
	int argc,
	char*const*argv,
	const char*optstring,
	const struct option*longopts,
	int*idx
){
	return __b_getlopt(argc,argv,optstring,longopts,idx,1);
}
