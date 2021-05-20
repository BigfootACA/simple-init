#ifndef STRING_H
#define STRING_H
#include<stdbool.h>
#include<sys/types.h>
#include"keyval.h"

// default time format
#define _DEFAULT_TIME_FORMAT "%Y/%m/%d %H:%M:%S"

#define UP_LETTER   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define UP_LETTERS  UP_LETTER " "
#define LOW_LETTER  "abcdefghijklmnopqrstuvwxyz"
#define LOW_LETTERS LOW_LETTER " "
#define LETTER      UP_LETTER LOW_LETTER
#define LETTERS     LETTER " "
#define NUMBER      "1234567890"
#define NUMBERS     NUMBER " "
#define VALID       LETTER NUMBER "_"

// src/lib/strings.c: format time with specified format (buffer size)
extern char*time2nstr(time_t*time,char*format,char*buff,size_t len);

// src/lib/strings.c: format time with specified format
extern char*time2str(time_t*time,char*format,char*buff);

// src/lib/strings.c: format time with default format (buffer size)
extern char*time2ndefstr(time_t*time,char*buff,size_t len);

// src/lib/strings.c: format time with default format
extern char*time2defstr(time_t*time,char*buff);

// src/lib/strings.c: malloc and init new string
extern char*new_string(size_t size);

// src/lib/strings.c: is 'x' in 'source'
extern bool contains_of(const char*source,size_t size,char x);

// src/lib/strings.c: 'source' only allows chars in 'valid' (buffer size)
extern bool check_nvalid(const char*source,size_t size_source,const char*valid,size_t size_valid);

// src/lib/strings.c: 'source' only allows chars in 'valid'
extern bool check_valid(char*source,const char*valid);

// src/lib/strings.c: 'source' only allows ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_ (buffer size)
extern bool check_nvalid_default(char*source,size_t size);

// src/lib/strings.c: 'source' only allows ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_
extern bool check_valid_default(char*source);

// src/lib/strings.c: convert decimal to hexadecimal
extern char dec2hex(int dec,bool upper);

// src/lib/strings.c: generate random hexadecimal string
extern char*gen_rand_hex(char*buff,int size,bool upper);

// src/lib/strings.c: replace char 'from' to 'to' in 'str'
extern char*strrep(char*str,char from,char to);

// src/lib/strings.c: repeat char 'c' to fd
extern int repeat(int fd,char c,size_t times);

// src/lib/mode.c: convert mode to mode string (eg: mode_string(0755)="rwxr-xr-x")
extern const char*mode_string(mode_t mode);

// src/lib/readable.c: convert number to readable size string (eg: make_readable_str(1048576,1,0)="1M")
extern const char* make_readable_str(unsigned long long val,unsigned long block_size,unsigned long display);

// src/lib/strings.c: parse string to long
extern long parse_long(char*str,long def);

// src/lib/strings.c: parse string to int
extern int parse_int(char*str,int def);

// src/lib/strings.c: skip input to stop in fd
extern char skips(int fd,char stop[]);

// src/lib/replace.c: replace holder (eg: replace([{"a","aa"}],'%',xxxx,'bb%abb',16)="bbaabb")
extern char*replace(keyval**table,char del,char*dest,char*src,size_t size);

// src/lib/strings.c: fuzzy compare string
extern bool fuzzy_cmp(const char*s1,const char*s2);

extern bool fuzzy_cmps(const char*v,const char**s);
#endif
