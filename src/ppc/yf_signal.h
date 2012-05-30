#ifndef  _YF_SIGNAL_H
#define _YF_SIGNAL_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

#define _yf_singal_helper(n)  SIG##n
#define yf_signal_value(n)      _yf_singal_helper(n)

#define YF_SHUTDOWN_SIGNAL      QUIT
#define YF_TERMINATE_SIGNAL     TERM
#define YF_NOACCEPT_SIGNAL      WINCH
#define YF_RECONFIGURE_SIGNAL   HUP

#define YF_SWITCH_LOG_SIGNAL   USR1
#define YF_CHANGEBIN_SIGNAL     USR2


typedef struct
{
        int   signo;
        char *signame;
        char *name;
        
        sig_atomic_t*  setval;
} yf_signal_t;

yf_int_t yf_init_signals(yf_log_t *log);
void yf_signal_handler(int signo);

yf_int_t yf_os_signal_process(char *name, yf_int_t pid, yf_log_t *log);
void yf_process_get_status();

#endif

