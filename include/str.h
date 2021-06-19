#ifndef STRING_H
#define STRING_H
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
#define VALID       LETTER NUMBER "_"
#define VALIDL      LETTER "_"
#define MONTH       "JanFebMarAprMayJunJulAugSepOctNovDec"
#define WEEK        "MonTueWedThuFriSatSun"

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

// src/lib/strings.c: fuzzy compare string array
extern bool fuzzy_cmps(const char*v,const char**s);

// src/lib/strings.c: match specified format and return matched length
extern size_t possible_match(char*src,poss**p);

// src/lib/strings.c: return max match length
extern size_t possible_length(poss**p);

// src/lib/strings.c: check string is a valid identifier
extern bool check_identifier(char*str);
#endif
