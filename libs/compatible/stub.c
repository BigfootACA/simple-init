#include <Library/DebugLib.h>
#include "ctype.h"
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "logger.h"
static int stub_stdin=0,stub_stdout=0,stub_stderr=0;
weak_decl FILE*const stdin=(FILE*)&stub_stdin;
weak_decl FILE*const stdout=(FILE*)&stub_stdout;
weak_decl FILE*const stderr=(FILE*)&stub_stderr;
weak_decl FILE *fopen(const char * file __attribute__((unused)), const char * mode __attribute__((unused))){
	DEBUG((EFI_D_ERROR,"try to call unsupported fopen\n"));
	errno=ENOSYS;
	return NULL;
}
weak_decl FILE *fdopen(int fildes, const char *mode){
	errno=ENOSYS;
	return NULL;
}
weak_decl FILE *freopen(const char *path, const char *mode, FILE *stream){
	errno=ENOSYS;
	return NULL;
}
weak_decl int fclose(FILE *f __attribute__((unused))){
	DEBUG((EFI_D_ERROR,"try to call unsupported fclose\n"));
	errno=ENOSYS;
	return -1;
}
weak_decl int fseek(FILE *f __attribute__((unused)), long p __attribute__((unused)), int m __attribute__((unused))){
	DEBUG((EFI_D_ERROR,"try to call unsupported fseek\n"));
	errno=ENOSYS;
	return -1;
}
weak_decl char*getenv(const char*name){
	return NULL;
}
weak_decl size_t fwrite(const void *restrict ptr, size_t size, size_t nmemb,FILE *restrict stream){
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
weak_decl int fprintf(FILE*stream,const char*format,...){
	char content[16384];
	if(!format||!stream)return -1;
	memset(content,0,sizeof(content));
	va_list ap;
	va_start(ap,format);
	if(!vsnprintf(content,sizeof(content)-1,format,ap))return -errno;
	va_end(ap);
	return fwrite(content,strlen(content),sizeof(char),stream);
}
weak_decl int printf(const char*format,...){
	char content[16384];
	if(!format)return -1;
	memset(content,0,sizeof(content));
	va_list ap;
	va_start(ap,format);
	if(!vsnprintf(content,sizeof(content)-1,format,ap))return -1;
	va_end(ap);
	return logger_print(LEVEL_INFO,"stdout",content);
}
weak_decl int feof(FILE *stream){
	errno=ENOSYS;
	return EOF;
}
weak_decl void clearerr(FILE *stream){
	errno=ENOSYS;
}
weak_decl int ferror(FILE *stream){
	errno=ENOSYS;
	return -1;
}
weak_decl int fflush(FILE *stream){
	errno=ENOSYS;
	return -1;
}
weak_decl int fileno(FILE *stream){
	errno=ENOSYS;
	return -1;
}
weak_decl int fgetc(FILE *stream){
	errno=ENOSYS;
	return EOF;
}
weak_decl int getc(FILE *stream){
	errno=ENOSYS;
	return EOF;
}
weak_decl int getchar(void){
	errno=ENOSYS;
	return EOF;
}
weak_decl char *fgets(char *restrict s, int size, FILE *restrict stream){
	errno=ENOSYS;
	return NULL;
}
weak_decl long ftell(FILE *stream){
	errno=ENOSYS;
	return -1;
}
weak_decl void rewind(FILE *stream){
	errno=ENOSYS;
}
weak_decl int fgetpos(FILE *restrict stream, fpos_t *restrict pos){
	errno=ENOSYS;
	return -1;
}
weak_decl int fsetpos(FILE *stream, const fpos_t *pos){
	errno=ENOSYS;
	return -1;
}
