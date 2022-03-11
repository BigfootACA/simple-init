#include <Library/DebugLib.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "logger.h"
static int stub_stdout=0,stub_stderr=0;
FILE*const stdout=(FILE*)&stub_stdout;
FILE*const stderr=(FILE*)&stub_stderr;
FILE *fopen(const char * file __attribute__((unused)), const char * mode __attribute__((unused))){
	DEBUG((EFI_D_ERROR,"try to call unsupported fopen\n"));
	errno=ENOSYS;
	return NULL;
}
int fclose(FILE *f __attribute__((unused))){
	DEBUG((EFI_D_ERROR,"try to call unsupported fclose\n"));
	errno=ENOSYS;
	return -1;
}
int fseek(FILE *f __attribute__((unused)), long p __attribute__((unused)), int m __attribute__((unused))){
	DEBUG((EFI_D_ERROR,"try to call unsupported fseek\n"));
	errno=ENOSYS;
	return -1;
}
char*getenv(const char*name){
	return NULL;
}
int fprintf(FILE*stream,const char*format,...){
	char*tag;
	char content[16384];
	enum log_level level;
	if(!format||!stream)return -1;
	if(stream==stdout)tag="stdout",level=LEVEL_INFO;
	else if(stream==stderr)tag="stderr",level=LEVEL_ERROR;
	else return -1;
	memset(content,0,sizeof(content));
	va_list ap;
	va_start(ap,format);
	if(!vsnprintf(content,sizeof(content)-1,format,ap))return -errno;
	va_end(ap);
	return logger_print(level,tag,content);
}
int printf(const char*format,...){
	char content[16384];
	if(!format)return -1;
	memset(content,0,sizeof(content));
	va_list ap;
	va_start(ap,format);
	if(!vsnprintf(content,sizeof(content)-1,format,ap))return -1;
	va_end(ap);
	return logger_print(LEVEL_INFO,"stdout",content);
}
