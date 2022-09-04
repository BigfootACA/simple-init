/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef STRING_H
#define STRING_H
#include<stdint.h>
#include<stdbool.h>
#include<sys/types.h>
#include"keyval.h"

// default time format
#define _DEFAULT_TIME_FORMAT "%Y/%m/%d %H:%M:%S"

#define UP_LETTER   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define UP_LETTERS  " " UP_LETTER
#define LOW_LETTER  "abcdefghijklmnopqrstuvwxyz"
#define LOW_LETTERS " " LOW_LETTER
#define LETTER      UP_LETTER LOW_LETTER
#define LETTERS     " " LETTER
#define NUMBER      "1234567890"
#define NUMBERS     " " NUMBER
#define UP_HEX      NUMBER"ABCDEF"
#define LOW_HEX     NUMBER"abcdef"
#define HEX         NUMBER"ABCDEFabcdef"
#define VALID       LETTER NUMBER "_"
#define VALIDL      LETTER "_"
#define MONTH       "JanFebMarAprMayJunJulAugSepOctNovDec"
#define WEEK        "MonTueWedThuFriSatSun"

extern const char*size_units[];

struct possibility{
	char*data;
	size_t data_len;
	size_t item_len;
	size_t possible;

};
typedef struct possibility poss;

#define POS_ITEM(data,item) &(poss){data,sizeof(data)-1,(item),(sizeof(data)-1)/(item)}
#define POS_LOW_LETTERS POS_ITEM(LOW_LETTERS,1)
#define POS_LOW_LETTER  POS_ITEM(LOW_LETTER,1)
#define POS_UP_LETTERS  POS_ITEM(UP_LETTERS,1)
#define POS_UP_LETTER   POS_ITEM(UP_LETTER,1)
#define POS_LETTERS     POS_ITEM(LETTERS,1)
#define POS_NUMBERS     POS_ITEM(NUMBERS,1)
#define POS_LETTER      POS_ITEM(LETTER,1)
#define POS_NUMBER      POS_ITEM(NUMBER,1)
#define POS_VALID       POS_ITEM(VALID,1)
#define POS_MONTH       POS_ITEM(MONTH,3)
#define POS_COLON       POS_ITEM(":",1)
#define POS_SPACE       POS_ITEM(" ",1)
#define POS_WEEK        POS_ITEM(WEEK,3)
#define POSS_TIME       POS_NUMBER,POS_NUMBER,POS_COLON,POS_NUMBER,POS_NUMBER,POS_COLON,POS_NUMBER,POS_NUMBER
#define POSS_RFC3164    POS_MONTH,POS_SPACE,POS_NUMBERS,POS_NUMBER,POS_SPACE,POSS_TIME
#define XPOS_TIME      (poss*[]){POSS_TIME,NULL}
#define XPOS_TIMES     (poss*[]){POSS_TIME,POS_SPACE,NULL}
#define XPOS_RFC3164   (poss*[]){POSS_RFC3164,NULL}
#define XPOS_RFC3164S  (poss*[]){POSS_RFC3164,POS_SPACE,NULL}
#define BOOL2STR(bool) ((bool)?"true":"false")

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

// src/lib/strings.c: convert hexadecimal to decimal
extern int hex2dec(char hex);

// src/lib/strings.c: convert binary data to hexadecimal string
extern char*bin2hexstr(char*buf,uint8_t*src,size_t len,bool upper);

// src/lib/strings.c: generate random hexadecimal string
extern char*gen_rand_hex(char*buff,int size,bool upper);

// src/lib/strings.c: replace char 'from' to 'to' in 'str'
extern char*strrep(char*str,char from,char to);

// src/lib/strings.c: count chars in string
extern size_t strcnt(const char*str,const char*chr);

// src/lib/strings.c: count chars in string
extern size_t strncnt(const char*str,size_t len,const char*chr);

// src/lib/strings.c: repeat char 'c' to fd
extern int repeat(int fd,char c,size_t times);

// src/lib/mode.c: convert mode to mode string (eg: mode_string(0755)="rwxr-xr-x")
extern const char*mode_string(mode_t mode);

// src/lib/readable.c: convert number to readable size string (eg: make_readable_str(1048576,1,0)="1M")
extern const char* make_readable_str(unsigned long long val,unsigned long block_size,unsigned long display);

// src/lib/readable.c: convert number to readable size string (eg: make_readable_str(1048576,1,0)="1M")
extern const char* make_readable_str_buf(char*buf,size_t len,unsigned long long val,unsigned long block_size,unsigned long display);

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

// src/lib/strings.c: fuzzy compare string array
extern bool fuzzy_cmps(const char*v,const char**s);

// src/lib/strings.c: match specified format and return matched length
extern size_t possible_match(char*src,poss**p);

// src/lib/strings.c: return max match length
extern size_t possible_length(poss**p);

// src/lib/strings.c: check string is a valid identifier
extern bool check_identifier(char*str);

// src/lib/strings.c: trim blank char in string start and end
extern void trim(char*str);

// src/lib/strings.c: convert string to upper case
extern void strtoupper(char*str);

// src/lib/strings.c: convert string to lower case
extern void strtolower(char*str);

// src/lib/strings.c: convert path string to an array
extern char**path2array(char*path,bool parent);

// src/lib/strings.c: remove slash from both sides of the string
extern char*trim_slash(char*path);

// src/lib/strings.c: append slash to string
extern char*add_right_slash(char*path,size_t len);

// src/lib/strings.c: convert buffer to hex string
extern char*buff2hex(char*hex,void*buff,size_t len);

// src/lib/strings.c: string is true
extern bool string_is_true(const char*string);

// src/lib/strings.c: string is false
extern bool string_is_false(const char*string);

// src/lib/strings.c: escape string
extern char*str_escape(const char*str);

// src/lib/strings.c: unescape string
extern char*str_unescape(const char*str);

// src/lib/strings.c: escape xml string
extern char*xml_escape(const char*str);

// src/lib/strings.c: unescape xml string
extern char*xml_unescape(const char*str);

#ifdef _LIST_H

// src/lib/strings.c: convert path string to a list
extern list*path2list(char*path,bool parent);

// src/lib/strings.c: simplify path components
extern list*path_simplify(list*paths,bool free);
#endif

// src/lib/strings.c: duplicate memory
#undef memdup
extern void*_memdup(void*mem,size_t len);
#define memdup _memdup

// src/lib/strings.c: string concatenation
#undef strlcat
extern size_t _strlcat(char*buf,const char*src,size_t len);
#define strlcat _strlcat

// src/lib/strings.c: string copy
#undef strlcpy
extern size_t _strlcpy(char*buf,const char*src,size_t len);
#define strlcpy _strlcpy

#ifdef b64_pton
#undef b64_pton
#endif

// src/lib/base64.c: base64 encode
extern int b64_pton(char const*src,unsigned char*target,size_t targsize);

// src/lib/random.c: get random number in range
extern int rand_get_number(int low_n,int high_n);

// src/lib/random.c: open random device
extern int random_get_fd(void);

// src/lib/random.c: generate random bytes
extern int random_get_bytes(void *buf, size_t nbytes);

// src/lib/mime.c: lookup by file ext name
extern char*mime_get_by_ext(char*buff,size_t bs,const char*ext);

// src/lib/mime.c: lookup by file name
extern char*mime_get_by_filename(char*buff,size_t bs,const char*filename);

// src/lib/strings.c: print and append string
extern size_t lsnprintf(char*buf,size_t len,const char*fmt,...);

// src/lib/strings.c: convert path to unix style and remove duplicate slashes
extern void trim_path(char*buf);

#endif
