#ifndef _YF_PROCESS_H
#define _YF_PROCESS_H

#include "yf_setproctitle.h"

typedef pid_t yf_pid_t;

#define YF_INVALID_PID  -1

#define YF_PROC_CHILD  -1
#define YF_PROC_POPEN_R -2
#define YF_PROC_POPEN_W -3
#define YF_PROC_POPEN_RW -3
#define YF_PROC_DETACH -5

#define yf_proc_writable(type) ((type==YF_PROC_POPEN_W) \
                || (type==YF_PROC_POPEN_RW))
#define yf_proc_readable(type) ((type==YF_PROC_POPEN_R) \
                || (type==YF_PROC_POPEN_RW))                

struct  yf_process_s;

typedef void (*yf_spawn_proc_pt)(void *data, yf_log_t *log);
typedef void (*yf_proc_exit_pt)(struct  yf_process_s* proc);

typedef struct  yf_process_s
{
        yf_pid_t         pid;
        yf_int_t          status;
        yf_socket_t      channel[2];

        yf_spawn_proc_pt proc;
        yf_proc_exit_pt  exit_cb;
        
        void *           data;
        const char *  name;

        //int with 1 width error
        yf_int_t         type : 8;
        yf_int_t         exiting : 2;//must 2 bits
        yf_int_t         exited : 2;
} 
yf_process_t;

/*
* if envp==NULL, then path can be relative path, else must abs... path
*/
typedef struct
{
        char *       path;
        const char * name;
        char *const *argv;
        char *const *envp;

        void * data;
        yf_proc_exit_pt  exit_cb;
        yf_int_t       type;
} 
yf_exec_ctx_t;


#define YF_MAX_PROCESSES         64

#if YF_MAX_PROCESSES >= 255
#error "too may processors!!"
#endif


#define yf_getpid   getpid


yf_int_t yf_init_processs(yf_log_t *log);

yf_pid_t yf_spawn_process(yf_spawn_proc_pt proc
                , void *data, const char *name, yf_int_t respawn
                , yf_proc_exit_pt  exit_cb
                , yf_log_t *log);

yf_pid_t yf_execute(yf_exec_ctx_t *ctx, yf_log_t *log);

yf_int_t yf_daemon(yf_log_t *log);

yf_int_t yf_pid_slot(yf_pid_t  pid);

/*
* used for child proc
*/
void yf_update_channel(yf_channel_t* channel, yf_log_t *log);


#ifdef  HAVE_SCHED_H
#include <sched.h>
#define yf_sched_yield()  sched_yield()
#else
#define yf_sched_yield()  yf_usleep(1)
#endif

extern yf_pid_t  yf_pid;
extern yf_socket_t  yf_channel;
extern yf_int_t  yf_process_slot;
extern yf_int_t  yf_last_process;
extern yf_process_t  yf_processes[YF_MAX_PROCESSES];

#define yf_log_pid  yf_pid

#endif

