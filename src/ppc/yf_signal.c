#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

static yf_log_t *sig_log;

extern  yf_signal_t  signals[];

yf_int_t
yf_init_signals(yf_log_t *log)
{
        sig_log = log;
        
        yf_signal_t *sig;
        struct sigaction sa;

        for (sig = signals; sig->signo != 0; sig++)
        {
                yf_memzero(&sa, sizeof(struct sigaction));

                if (sig->setval == NULL)
                        sa.sa_handler = SIG_IGN;
                else
                        sa.sa_handler = yf_signal_handler;
                
                sigemptyset(&sa.sa_mask);
                
                if (sigaction(sig->signo, &sa, NULL) == -1)
                {
                        yf_log_error(YF_LOG_EMERG, sig_log, yf_errno,
                                     "sigaction(%s) failed", sig->signame);
                        return YF_ERROR;
                }
        }

        return YF_OK;
}


void
yf_signal_handler(int signo)
{
        yf_int_t ignore;
        yf_err_t err;
        yf_signal_t *sig;

        ignore = 0;

        err = yf_errno;

        for (sig = signals; sig->signo != 0; sig++)
        {
                if (sig->signo == signo)
                {
                        break;
                }
        }

        *sig->setval = 1;
        
        /*yf_log_error(YF_LOG_NOTICE, sig_log, 0,
                     "signal %d (%s) received", signo, sig->signame);

        if (signo == SIGCHLD)
        {
                yf_process_get_status();
        }*/

        yf_set_errno(err);
}


void
yf_process_get_status()
{
        int status;
        char *process;
        yf_pid_t pid;
        yf_err_t err;
        yf_int_t i;
        yf_uint_t one;

        one = 0;

        for (;; )
        {
                pid = waitpid(-1, &status, WNOHANG);

                if (pid == 0)
                {
                        return;
                }

                if (pid == -1)
                {
                        err = yf_errno;

                        if (err == YF_EINTR)
                        {
                                continue;
                        }

                        if (err == YF_ECHILD && one)
                        {
                                return;
                        }

                        yf_log_error(YF_LOG_ALERT, sig_log, err,
                                     "waitpid() failed");
                        return;
                }
                
                one = 1;
                process = "unknown process";

                for (i = 0; i < yf_last_process; i++)
                {
                        if (yf_processes[i].pid == pid)
                        {
                                yf_processes[i].status = status;
                                yf_processes[i].exited = 1;
                                process = yf_processes[i].name;
                                break;
                        }
                }

                if (WTERMSIG(status))
                {
#ifdef WCOREDUMP
                        yf_log_error(YF_LOG_ALERT, sig_log, 0,
                                     "%s %P exited on signal %d%s",
                                     process, pid, WTERMSIG(status),
                                     WCOREDUMP(status) ? " (core dumped)" : "");
#else
                        yf_log_error(YF_LOG_ALERT, sig_log, 0,
                                     "%s %P exited on signal %d",
                                     process, pid, WTERMSIG(status));
#endif
                }
                else {
                        yf_log_error(YF_LOG_NOTICE, sig_log, 0,
                                     "%s %P exited with code %d",
                                     process, pid, WEXITSTATUS(status));
                }

                if (WEXITSTATUS(status) == 2 && yf_processes[i].respawn)
                {
                        yf_log_error(YF_LOG_ALERT, sig_log, 0,
                                     "%s %P exited with fatal code %d "
                                     "and can not be respawn",
                                     process, pid, WEXITSTATUS(status));
                        yf_processes[i].respawn = 0;
                }
        }
}


yf_int_t
yf_os_signal_process(char *name, yf_int_t pid, yf_log_t *log)
{
        yf_signal_t *sig;

        for (sig = signals; sig->signo != 0; sig++)
        {
                if (yf_strcmp(name, sig->name) == 0)
                {
                        if (kill(pid, sig->signo) == 0)
                        {
                                yf_log_debug2(YF_LOG_DEBUG, log, 0,
                                             "kill (%P, %d) success", pid, sig->signo);
                                return 0;
                        }

                        yf_log_error(YF_LOG_ALERT, log, yf_errno,
                                     "kill(%P, %d) failed", pid, sig->signo);
                }
        }

        return 1;
}


