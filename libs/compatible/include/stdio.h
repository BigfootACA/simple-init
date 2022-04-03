#ifndef _STDIO_H
#define _STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "compatible.h"
#define __NEED_FILE
#define __NEED___isoc_va_list
#define __NEED_size_t

#if __STDC_VERSION__ < 201112L
#define __NEED_struct__IO_FILE
#endif

#define __NEED_ssize_t
#define __NEED_off_t
#define __NEED_va_list

#include <bits/alltypes.h>
#define MB_LEN_MAX  4
#define __SLBF  0x0001    /**< line buffered */
#define __SNBF  0x0002    /**< unbuffered */
#define __SRD   0x0004    /**< OK to read */
#define __SWR   0x0008    /**< OK to write */
/* RD and WR are never simultaneously asserted */
#define __SRW   0x0010    /**< open for reading & writing */
#define __SEOF  0x0020    /**< found EOF */
#define __SERR  0x0040    /**< found error */
#define __SMBF  0x0080    /**< _buf is from malloc */
#define __SAPP  0x0100    /**< fdopen()ed in append mode */
#define __SSTR  0x0200    /**< this is an sprintf/snprintf string */
#define __SOPT  0x0400    /**< do fseek() optimization */
#define __SNPT  0x0800    /**< do not do fseek() optimization */
#define __SOFF  0x1000    /**< set iff _offset is in fact correct */
#define __SMOD  0x2000    /**< true => fgetln modified _p text */
#define __SALC  0x4000    /**< allocate string space dynamically */
#define __sferror(p)    (((p)->_flags & __SERR) != 0)
#undef EOF
#define EOF (-1)

#undef SEEK_SET
#undef SEEK_CUR
#undef SEEK_END
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define BUFSIZ 1024
#define FILENAME_MAX 4096
#define FOPEN_MAX 1000
#define TMP_MAX 10000
#define L_tmpnam 20

typedef union _G_fpos64_t {
	char __opaque[16];
	long long __lldata;
	double __align;
} fpos_t;
/* stdio buffers */
struct __sbuf {
	unsigned char *_base;
	int _size;
};
typedef struct __sFILE {
	unsigned char  *_p;         /**< current position in (some) buffer */
	int             _r;         /**< read space left for getc() */
	int             _w;         /**< write space left for putc() */
	unsigned short  _flags;     /**< flags, below; this FILE is free if 0 */
	short           _file;      /**< fileno, if Unix descriptor, else -1 */
	struct  __sbuf  _bf;        /**< the buffer (at least 1 byte, if !NULL) */
	int             _lbfsize;   /**< 0 or -_bf._size, for inline putc */

		/* operations */
	void           *_cookie;    /**< cookie passed to io functions */
	int           (*_close)(void *);
	int           (*_read) (void *, char *, int);
	fpos_t        (*_seek) (void *, fpos_t, int);
	int           (*_write)(void *, const char *, int);

	/** file extension */
	struct  __sbuf  _ext;

	/** @{
	    Separate buffer for long sequences of ungetc().
	**/
	unsigned char  *_up;        /**< saved _p when _p is doing ungetc data */
	int             _ur;        /**< saved _r when _r is counting ungetc data */
	/*@}*/

	/* tricks to meet minimum requirements even when malloc() fails */
	unsigned char   _ubuf[3 * MB_LEN_MAX];   /**< guarantee an ungetc() buffer */
	unsigned char   _nbuf[1 * MB_LEN_MAX];   /**< guarantee a getc() buffer */

	/** separate buffer for fgetln() when line crosses buffer boundary */
	struct  __sbuf  _lb;        /* buffer for fgetln() */

	/* Unix stdio files get aligned to block boundaries on fseek() */
	int             _blksize;   /**< stat.st_blksize (may be != _bf._size) */
	fpos_t          _offset;    /**< current lseek offset */
} FILE;

extern FILE *const stdin;
extern FILE *const stdout;
extern FILE *const stderr;

FILE *fopen(const char *__restrict, const char *__restrict);
FILE *freopen(const char *__restrict, const char *__restrict, FILE *__restrict);
int fclose(FILE *);

int remove(const char *);
int rename(const char *, const char *);

int feof(FILE *);
int ferror(FILE *);
int fflush(FILE *);
void clearerr(FILE *);

int fseek(FILE *, long, int);
long ftell(FILE *);
void rewind(FILE *);

int fgetpos(FILE *__restrict, fpos_t *__restrict);
int fsetpos(FILE *, const fpos_t *);

size_t fread(void *__restrict, size_t, size_t, FILE *__restrict);
size_t fwrite(const void *__restrict, size_t, size_t, FILE *__restrict);

int fgetc(FILE *);
int getc(FILE *);
int getchar(void);
int ungetc(int, FILE *);

int fputc(int, FILE *);
int putc(int, FILE *);
int putchar(int);

char *fgets(char *__restrict, int, FILE *__restrict);
#if __STDC_VERSION__ < 201112L
char *gets(char *);
#endif

int fputs(const char *__restrict, FILE *__restrict);
int puts(const char *);

int printf(const char *__restrict, ...);
int fprintf(FILE *__restrict, const char *__restrict, ...);
int sprintf(char *__restrict, const char *__restrict, ...);
int snprintf(char *__restrict, size_t, const char *__restrict, ...);

int vprintf(const char *__restrict, __isoc_va_list);
int vfprintf(FILE *__restrict, const char *__restrict, __isoc_va_list);
int vsprintf(char *__restrict, const char *__restrict, __isoc_va_list);
int vsnprintf(char *__restrict, size_t, const char *__restrict, __isoc_va_list);

int vasprintf(char ** __restrict, const char * __restrict, __isoc_va_list);

int scanf(const char *__restrict, ...);
int fscanf(FILE *__restrict, const char *__restrict, ...);
int sscanf(const char *__restrict, const char *__restrict, ...);
int vscanf(const char *__restrict, __isoc_va_list);
int vfscanf(FILE *__restrict, const char *__restrict, __isoc_va_list);
int vsscanf(const char *__restrict, const char *__restrict, __isoc_va_list);

void perror(const char *);

int setvbuf(FILE *__restrict, char *__restrict, int, size_t);
void setbuf(FILE *__restrict, char *__restrict);

char *tmpnam(char *);
FILE *tmpfile(void);

#ifdef __cplusplus
}
#endif

#endif
