#ifndef STRING_H
#define STRING_H
#include<stdbool.h>
#include<sys/types.h>

// default time format
#define _DEFAULT_TIME_FORMAT "%Y/%m/%d %H:%M:%S"

// src/lib/strings.h: format time with specified format (buffer size)
extern char*time2nstr(time_t*time,char*format,char*buff,size_t len);

// src/lib/strings.h: format time with specified format
extern char*time2str(time_t*time,char*format,char*buff);

// src/lib/strings.h: format time with default format (buffer size)
extern char*time2ndefstr(time_t*time,char*buff,size_t len);

// src/lib/strings.h: format time with default format
extern char*time2defstr(time_t*time,char*buff);

// src/lib/strings.h: malloc and init new string
extern char*new_string(size_t size);

// src/lib/strings.h: is 'x' in 'source'
extern bool contains_of(const char*source,size_t size,char x);

// src/lib/strings.h: 'source' only allows chars in 'valid' (buffer size)
extern bool check_nvalid(const char*source,size_t size_source,const char*valid,size_t size_valid);

// src/lib/strings.h: 'source' only allows chars in 'valid'
extern bool check_valid(char*source,const char*valid);

// src/lib/strings.h: 'source' only allows ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_ (buffer size)
extern bool check_nvalid_default(char*source,size_t size);

// src/lib/strings.h: 'source' only allows ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_
extern bool check_valid_default(char*source);

// src/lib/strings.h: convert decimal to hexadecimal
extern char dec2hex(int dec,bool upper);

// src/lib/strings.h: generate random hexadecimal string
extern char*gen_rand_hex(char*buff,int size,bool upper);

// src/lib/strings.h: replace char 'from' to 'to' in 'str'
extern char*strrep(char*str,char from,char to);

// src/lib/strings.h: repeat char 'c' to fd
extern int repeat(int fd,char c,size_t times);

// src/lib/mode.h: convert mode to mode string (eg: mode_string(0755)="rwxr-xr-x")
extern const char*mode_string(mode_t mode);

// src/lib/readable.h: convert number to readable size string (eg: make_readable_str(1048576,1,0)="1M")
extern const char* make_readable_str(unsigned long long val,unsigned long block_size,unsigned long display);

// src/lib/strings.h: parse string to long
extern long parse_long(char*str,long def);

// src/lib/strings.h: parse string to int
extern int parse_int(char*str,int def);

// src/lib/strings.h: skip input to stop in fd
extern char skips(int fd,char stop[]);

#endif
