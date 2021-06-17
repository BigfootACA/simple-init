#ifndef INIT_H
#define INIT_H
#include"logger.h"
#include"defines.h"

// src/initd/init.c: simple-init daemon main
extern int init_main(int argc,char**argv);

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
extern int fork_run(char*tag,bool wait,pid_t*p,void*data,runnable_t*runnable);

// src/initd/run.c: run a program once
extern pid_t run_program(char*path,char**args);

// src/initd/switchroot.c: switch to new root
extern int run_switch_root(char*root,char*init);

// src/initd/switchroot.c: check init is valid
extern bool check_init(bool force,char*root,char*init);

// src/initd/switchroot.c: search init in root when init is not null
extern char*search_init(char*init,char*root);

// src/initd/run.c: wait for a PID exit with exit code
extern int wait_cmd(pid_t p);

// src/initd/signal.c: init setup signals to signal_handlers
extern void setup_signals();

// src/initd/signal.c: disable init signal handlers
extern void disable_signals();

// src/initd/environ.c: 
extern void init_environ();

// src/initd/umount.c: umount all mountpoints
extern int umount_all();

// src/initd/preinit.c: simple-init preinit
extern int preinit();

// src/initd/logfs.c: setup logfs
extern int setup_logfs();

// src/initd/logfs.c: wait logfs setup done
extern int wait_logfs();

// src/initd/reboot.c: init call reboot
extern int call_reboot(long rb,char*cmd);

// src/initd/reboot.c: init kill all processes
extern int kill_all();

// src/initd/client.c: init control socket fd
extern int initfd;

// src/initd/client.c: set initfd
extern int set_initfd(int fd);

// src/initd/client.c: close initfd
extern void close_initfd();

// src/initd/client.c: connect to a init control socket
extern int open_socket_initfd(char*path);

#ifdef INIT_INTERNAL_H
// src/initd/client.c: send init_msg
extern int init_send_raw(struct init_msg*send);

// src/initd/client.c: receive init_msg
extern int init_recv_raw(struct init_msg*response);

// src/initd/client.c: send init_msg and wait response
extern int init_send(struct init_msg*send,struct init_msg*response);
#endif
#endif
