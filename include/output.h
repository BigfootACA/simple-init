/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef OUTPUT_H
#define OUTPUT_H
#include<stdarg.h>
#include<unistd.h>
#include<stdbool.h>
#include<sys/types.h>

// src/lib/exit.c: output formatted error to stderr and return
extern int ret_perror(int err,bool quit,const char*format,...) __attribute__((format(printf,3,4)));;

// src/lib/exit.c: output formatted error to stdout and return
extern int ret_stdout_perror(int err,bool quit,const char*format,...) __attribute__((format(printf,3,4)));;

// src/lib/exit.c: output formatted text and return
extern int ret_printf(int err,bool quit,int fd,const char*format,...) __attribute__((format(printf,4,5)));;

// src/lib/exit.c: output error message to fd with va_list
extern void fd_vperror(int fd,const char*format,va_list a);

// src/lib/exit.c: output formatted error message to fd
extern void fd_perror(int fd,const char*format,...) __attribute__((format(printf,2,3)));;

// output formatted error message to stdout
#define stdout_perror(s...)                 fd_perror(STDOUT_FILENO,s)

// output formatted error message to stderr
#define stderr_perror(s...)                 fd_perror(STDERR_FILENO,s)

// alias of ret_perror
#define ret_stderr_perror                   ret_perror

// output formatted error to stderr and return
#define return_stderr_perror(err,s...)      ret_stderr_perror(err,false,s)

// output formatted error to stderr and exit
#define exit_stderr_perror(err,s...)        ret_stderr_perror(err,true,s)

// output formatted error to stdout and return
#define return_stdout_perror(err,s...)      ret_stdout_perror(err,false,s)

// output formatted error to stdout and exit
#define exit_stdout_perror(err,s...)        ret_stdout_perror(err,true,s)

// output formatted error to stderr and return
#define return_perror(err,s...)             ret_perror(err,false,s)

// output formatted error to stderr and exit
#define exit_perror(err,s...)               ret_perror(err,true,s)

// output formatted text to fd and return
#define return_printf(err,fd,format...)     ret_printf(err,false,fd,format)

// output formatted text to fd and exit
#define exit_printf(err,fd,format...)       ret_printf(err,true,fd,format)

// output formatted text to stdout and return
#define return_stdout_printf(err,format...) ret_printf(err,false,STDOUT_FILENO,format)

// output formatted text to stdout and exit
#define exit_stdout_printf(err,format...)   ret_printf(err,true,STDOUT_FILENO,format)

// output formatted text to stderr and return
#define return_stderr_printf(err,format...) ret_printf(err,false,STDERR_FILENO,format)

// output formatted text to stderr and exit
#define exit_stderr_printf(err,format...)   ret_printf(err,true,STDERR_FILENO,format)

// output formatted error to stderr and return
#define re_err(err,s...)                    ret_stderr_perror(err,false,s)

// output formatted error to stderr and exit
#define ee_err(err,s...)                    ret_stderr_perror(err,true,s)

// output formatted error to stdout and return
#define ro_err(err,s...)                    ret_stdout_perror(err,false,s)

// output formatted error to stdout and exit
#define eo_err(err,s...)                    ret_stdout_perror(err,true,s)

// output formatted error to stderr and return
#define r_err(err,s...)                     ret_perror(err,false,s)

// output formatted error to stderr and exit
#define e_err(err,s...)                     ret_perror(err,true,s)

// output formatted text to fd and return
#define r_printf(err,fd,format...)          ret_printf(err,false,fd,format)

// output formatted text to fd and exit
#define e_printf(err,fd,format...)          ret_printf(err,true,fd,format)

// output formatted text to stdout and return
#define ro_printf(err,format...)            ret_printf(err,false,STDOUT_FILENO,format)

// output formatted text to stdout and exit
#define eo_printf(err,format...)            ret_printf(err,true,STDOUT_FILENO,format)

// output formatted text to stderr and return
#define re_printf(err,format...)            ret_printf(err,false,STDERR_FILENO,format)

// output formatted text to stderr and exit
#define ee_printf(err,format...)            ret_printf(err,true,STDERR_FILENO,format)
#endif
