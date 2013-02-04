#ifndef  _YF_SIGNAL_H
#define _YF_SIGNAL_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

#define _yf_singal_helper(n)  SIG##n
#define yf_signal_value(n)      _yf_singal_helper(n)

//perform a core dump
#define YF_SHUTDOWN_SIGNAL      QUIT
//SIGTERM is the default signal sent to a process by the kill or killall commands
#define YF_TERMINATE_SIGNAL     TERM
#define YF_NOACCEPT_SIGNAL      WINCH
#define YF_RECONFIGURE_SIGNAL   HUP
#define YF_KILL_SIGNAL  KILL

#define YF_SWITCH_LOG_SIGNAL   USR1
#define YF_CHANGEBIN_SIGNAL     USR2


typedef void (*signal_ptr)(int signo);


typedef struct
{
        int   signo;
        char *signame;
        char *name;

        signal_ptr sig_handler;
} 
yf_signal_t;

yf_int_t yf_replace_sig_handler(int signo, signal_ptr sig_handler
                , signal_ptr* old_handler, yf_log_t *log);
yf_int_t yf_set_sig_handler(int signo, signal_ptr sig_handler, yf_log_t *log);

yf_int_t yf_os_signal_process(yf_signal_t* signals
                , char *name, yf_int_t pid, yf_log_t *log);

//kill(getpid(), signo)
void  yf_kill_exit(yf_int_t signo);

void yf_process_get_status(yf_log_t *log);

#define yf_proc_exit_err(status) (WIFEXITED(status) == 0)
#define yf_proc_exit_code(status) WEXITSTATUS(status)

#endif

