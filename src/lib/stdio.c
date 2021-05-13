#include<errno.h>
#include<stdio.h>
#include<stdarg.h>
#include<string.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<sys/resource.h>
#include"defines.h"
#include"output.h"

void fd_vperror(int fd,const char*format,va_list a){
	char buff[BUFFER_SIZE];
	memset(&buff,0,BUFFER_SIZE);
	if(format&&a)vsnprintf(buff,BUFFER_SIZE-1,format,a);
	dprintf(fd,errno!=0?"%s: %m\n":"%s.\n",buff);
}

void fd_perror(int fd,const char*format,...){
	va_list a;
	if(format)va_start(a,format);
	fd_vperror(fd,format,a);
	if(format)va_end(a);
}

int get_term_width(int fd,int def){
	int c=def;
	#if defined(TIOCGWINSZ)
	struct winsize w_win;
        if(ioctl(fd,TIOCGWINSZ,&w_win)==0)return w_win.ws_col;
	#elif defined(TIOCGSIZE)
	struct ttysize t_win;
        if(ioctl(fd,TIOCGSIZE,&t_win)==0)return t_win.ts_cols;
	#else
	(void)fd;
	#endif
        return c;
}

char skips(int fd,char stop[]){
	if(fd<0)return -EINVAL;
	size_t a;
	char bit[2]={0,0};
	while(read(fd,&bit,1)==1){
		a=0;
		while(a<strlen(stop))if(stop[a++]==bit[0])goto re;
	}
	re:
	errno=0;
	return bit[0];
}

int get_max_fd(){
        struct rlimit rl;
        rlim_t m;
        if(getrlimit(RLIMIT_NOFILE,&rl)<0)return -errno;
        m=MAX(rl.rlim_cur,rl.rlim_max);
        if(m<FD_SETSIZE)return FD_SETSIZE-1;
        if(m==RLIM_INFINITY||m>INT_MAX)return INT_MAX;
        return (int)(m-1);
}
