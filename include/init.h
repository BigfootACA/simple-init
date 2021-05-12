#ifndef INIT_H
#define INIT_H
#include"logger.h"
#include"defines.h"

// src/initd/init.c: simple-init daemon main
extern int init_main(int argc,char**argv);

// src/initd/signals.c: init signals handler
extern void signal_handlers(int s);

// src/initd/environ.c: dump environment variables to fd
extern void dump_environ(int fd);

// src/initd/environ.c: dump environment variables to file
extern void dump_environ_to_file(char*path);

// src/initd/environ.c: dump environment variables to /tmp/environ.PID.txt
extern void auto_dump_environ_file();

// src/initd/environ.c: dump environment variables to loggerd
extern void log_environ(enum log_level level,char*tag);

// src/initd/run.c: loop run a program (auto restart)
extern int run_loop_program(char*path,char**args);

// src/initd/run.c: fork and run a function
extern int fork_run(char*tag,bool wait,void*data,runnable_t*runnable);

// src/initd/run.c: run a program once
extern pid_t run_program(char*path,char**args);

// src/initd/switchroot.c: switch to new root
extern int run_switch_root(char*root,char*init);

// src/initd/run.c: wait for a PID exit with exit code
extern int wait_cmd(pid_t p);

// src/initd/signal.c: init setup signals to signal_handlers
extern void setup_signals();

// src/initd/environ.c: 
extern void init_environ();

// src/initd/umount.c: umount all mountpoints
extern int umount_all();

// src/initd/preinit.c: simple-init preinit
extern int preinit();
#endif