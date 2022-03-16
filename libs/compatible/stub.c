#include <Library/DebugLib.h>
#include "ctype.h"
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "logger.h"
static int stub_stdin=0,stub_stdout=0,stub_stderr=0;
FILE*const stdin=(FILE*)&stub_stdin;
FILE*const stdout=(FILE*)&stub_stdout;
FILE*const stderr=(FILE*)&stub_stderr;
FILE *fopen(const char * file __attribute__((unused)), const char * mode __attribute__((unused))){
	DEBUG((EFI_D_ERROR,"try to call unsupported fopen\n"));
	errno=ENOSYS;
	return NULL;
}
FILE *fdopen(int fildes, const char *mode){
	errno=ENOSYS;
	return NULL;
}
FILE *freopen(const char *path, const char *mode, FILE *stream){
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
size_t fwrite(const void *restrict ptr, size_t size, size_t nmemb,FILE *restrict stream){
	char*tag,*buf=(char*)ptr;
	enum log_level level;
	if(!buf||!stream)return -1;
	if(stream==stdout)tag="stdout",level=LEVEL_INFO;
	else if(stream==stderr)tag="stderr",level=LEVEL_ERROR;
	else return -1;
	while(*buf&&isspace(*buf))buf++;
	if(!*buf)return 0;
	return logger_print(level,tag,buf);
}
int fprintf(FILE*stream,const char*format,...){
	char content[16384];
	if(!format||!stream)return -1;
	memset(content,0,sizeof(content));
	va_list ap;
	va_start(ap,format);
	if(!vsnprintf(content,sizeof(content)-1,format,ap))return -errno;
	va_end(ap);
	return fwrite(content,strlen(content),sizeof(char),stream);
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
int feof(FILE *stream){
	errno=ENOSYS;
	return EOF;
}
void clearerr(FILE *stream){
	errno=ENOSYS;
}
int ferror(FILE *stream){
	errno=ENOSYS;
	return -1;
}
int fflush(FILE *stream){
	errno=ENOSYS;
	return -1;
}
int fileno(FILE *stream){
	errno=ENOSYS;
	return -1;
}
int fgetc(FILE *stream){
	errno=ENOSYS;
	return EOF;
}
int getc(FILE *stream){
	errno=ENOSYS;
	return EOF;
}
int getchar(void){
	errno=ENOSYS;
	return EOF;
}
char *fgets(char *restrict s, int size, FILE *restrict stream){
	errno=ENOSYS;
	return NULL;
}
size_t fread(void *restrict ptr, size_t size, size_t nmemb,FILE *restrict stream){
	errno=ENOSYS;
	return 0;
}
long ftell(FILE *stream){
	errno=ENOSYS;
	return -1;
}
void rewind(FILE *stream){
	errno=ENOSYS;
}
int fgetpos(FILE *restrict stream, fpos_t *restrict pos){
	errno=ENOSYS;
	return -1;
}
int fsetpos(FILE *stream, const fpos_t *pos){
	errno=ENOSYS;
	return -1;
}
