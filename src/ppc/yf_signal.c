#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

yf_int_t yf_replace_sig_handler(int signo, signal_ptr sig_handler
                , signal_ptr* old_handler, yf_log_t *log)
{
        struct sigaction sa, sa_old;
        yf_memzero(&sa, sizeof(struct sigaction));
        sa.sa_handler = sig_handler;

        sigemptyset(&sa.sa_mask);

#ifdef  SA_RESTART
        //sa.sa_flags |= SA_RESTART;
#endif
        
        if (sigaction(signo, &sa, &sa_old) == -1)
        {
                yf_log_error(YF_LOG_EMERG, log, yf_errno,
                             "sigaction signo(%d) failed", signo);
                return YF_ERROR;
        }

        if (old_handler)
                *old_handler = sa_old.sa_handler;
        return YF_OK;
}

yf_int_t yf_set_sig_handler(int signo, signal_ptr sig_handler, yf_log_t *log)
{
        return  yf_replace_sig_handler(signo, sig_handler, NULL, log);
}

void
yf_process_get_status(yf_log_t *log)
{
        yf_u8_t  proc_slot[YF_MAX_PROCESSES] = {0};
        yf_u8_t  max_size = YF_MAX_PROCESSES, exit_size = 0;
        
        int status;
        char *process;
        yf_pid_t pid;
        yf_err_t err;
        yf_int_t i;
        yf_uint_t one;
        yf_process_t *proc;

        one = 0;

        while (exit_size < max_size)
        {
                pid = waitpid(-1, &status, WNOHANG);
                yf_log_debug1(YF_LOG_DEBUG, log, 0, "waitpid ret=%P", pid);

                if (pid == 0)
                {
                        goto end;
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
                                goto end;
                        }

                        yf_log_error(YF_LOG_ALERT, log, err,
                                     "waitpid() failed");
                        goto end;
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
                
                proc_slot[exit_size++] = i;

                if (WTERMSIG(status))
                {
#ifdef WCOREDUMP
                        yf_log_error(YF_LOG_ALERT, log, 0,
                                     "%s %P exited on signal %d%s",
                                     process, pid, WTERMSIG(status),
                                     WCOREDUMP(status) ? " (core dumped)" : "");
#else
                        yf_log_error(YF_LOG_ALERT, log, 0,
                                     "%s %P exited on signal %d",
                                     process, pid, WTERMSIG(status));
#endif
                }
                else {
                        yf_log_error(YF_LOG_NOTICE, log, 0,
                                     "%s %P exited with code %d",
                                     process, pid, WEXITSTATUS(status));
                }
        }

end:
        for (i = 0; i < exit_size; ++i)
        {
                proc = yf_processes + proc_slot[i];
                if (proc->exit_cb)
                        proc->exit_cb(proc);
        }
}


yf_int_t
yf_os_signal_process(yf_signal_t* signals
                , const char *name, yf_int_t pid, yf_log_t *log)
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

void  yf_kill_exit(yf_int_t signo)
{
        struct sigaction sa;
        yf_memzero(&sa, sizeof(struct sigaction));
        sa.sa_handler = SIG_DFL;

        sigemptyset(&sa.sa_mask);
        sigaction(signo, &sa, NULL);

        kill(getpid(), signo);
}


