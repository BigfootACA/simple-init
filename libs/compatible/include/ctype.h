//
// Created by bigfoot on 2022/2/8.
//

#ifndef SIMPLE_INIT_CTYPE_H
#define SIMPLE_INIT_CTYPE_H
int comp_isalnum(int);
int comp_isalpha(int);
int comp_isblank(int);
int comp_iscntrl(int);
int comp_isdigit(int);
int comp_isgraph(int);
int comp_islower(int);
int comp_isprint(int);
int comp_ispunct(int);
int comp_isspace(int);
int comp_isupper(int);
int comp_isxdigit(int);
int comp_tolower(int);
int comp_toupper(int);
#define isalnum(i)comp_isalnum(i)
#define isalpha(i)comp_isalpha(i)
#define isblank(i)comp_isblank(i)
#define iscntrl(i)comp_iscntrl(i)
#define isdigit(i)comp_isdigit(i)
#define isgraph(i)comp_isgraph(i)
#define islower(i)comp_islower(i)
#define isprint(i)comp_isprint(i)
#define ispunct(i)comp_ispunct(i)
#define isspace(i)comp_isspace(i)
#define isupper(i)comp_isupper(i)
#define isxdigit(i)comp_isxdigit(i)
#define tolower(i)comp_tolower(i)
#define toupper(i)comp_toupper(i)
#endif //SIMPLE_INIT_CTYPE_H
