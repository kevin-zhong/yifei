#ifndef _YF_PROCESS_H
#define _YF_PROCESS_H

#include "yf_setproctitle.h"

typedef pid_t yf_pid_t;

#define YF_INVALID_PID  -1

typedef void (*yf_spawn_proc_pt)(void *data, yf_log_t *log);

typedef struct
{
        yf_pid_t         pid;
        int                 work_type;
        int                 status;
        yf_socket_t      channel[2];

        yf_spawn_proc_pt proc;
        void *           data;
        char *           name;

        unsigned         respawn : 1;
        unsigned         just_spawn : 1;
        unsigned         detached : 1;
        unsigned         exiting : 1;
        unsigned         exited : 1;
} yf_process_t;


typedef struct
{
        char *       path;
        char *       name;
        char *const *argv;
        char *const *envp;
} yf_exec_ctx_t;


#define YF_MAX_PROCESSES         64

#define YF_PROCESS_NORESPAWN     -1
#define YF_PROCESS_JUST_SPAWN    -2
#define YF_PROCESS_RESPAWN       -3
#define YF_PROCESS_JUST_RESPAWN  -4
#define YF_PROCESS_DETACHED      -5


#define yf_getpid   getpid


yf_int_t yf_init_processs(yf_log_t *log);

yf_pid_t yf_spawn_process(yf_spawn_proc_pt proc, void *data, char *name, yf_int_t respawn, yf_log_t *log);
yf_pid_t yf_execute(yf_exec_ctx_t *ctx, yf_log_t *log);

yf_int_t yf_daemon(yf_log_t *log);

void yf_update_channel(yf_channel_t* channel, yf_log_t *log);


#ifdef  HAVE_SCHED_H
#include <sched.h>
#define yf_sched_yield()  sched_yield()
#else
#define yf_sched_yield()  usleep(1)
#endif

extern yf_pid_t  yf_pid;
extern yf_socket_t  yf_channel;
extern yf_int_t  yf_process_slot;
extern yf_int_t  yf_last_process;
extern yf_process_t  yf_processes[YF_MAX_PROCESSES];

#define yf_log_pid  yf_pid

#endif

