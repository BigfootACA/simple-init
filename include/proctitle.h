#ifndef PROCTITLE_H
#define PROCTITLE_H

// src/lib/proctitle.c: copy environ
extern int spt_copyenv(char*oldenv[]);

// src/lib/proctitle.c: copy argvs
extern int spt_copyargs(int argc,char*argv[]);

// src/lib/proctitle.c: init proctitle
extern void spt_init(int argc,char*argv[]);

// src/lib/proctitle.c: set proctitle
extern void setproctitle(const char*fmt,...);
#endif
