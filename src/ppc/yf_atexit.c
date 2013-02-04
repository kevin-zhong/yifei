#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

static yf_lock_t _atexit_lock = YF_LOCK_INITIALIZER;
static yf_int_t  _atexit_called = 0;
static yf_atexit_handler  _atexit_callbacks[32] = {NULL};

static void  _call_atexit(void)
{
        yf_u32_t i1 = 0;
        if (_atexit_called)
                return;
        
        _atexit_called = 1;

        for ( i1 = 0; i1 < YF_ARRAY_SIZE(_atexit_callbacks); i1++ )
        {
                if (_atexit_callbacks[i1] == NULL)
                        break;
                _atexit_callbacks[i1]();
        }
}

void yf_exit_with_sig(int signo)
{
        _call_atexit();
        yf_kill_exit(signo);
}


yf_int_t  yf_atexit_handler_add(yf_atexit_handler pfn, yf_log_t* log)
{
        yf_u32_t i1 = 0, i2 = 0;
        yf_int_t  ret = 0;
        yf_lock(&_atexit_lock);
        
        for ( i1 = 0; i1 < YF_ARRAY_SIZE(_atexit_callbacks); i1++ )
        {
                if (_atexit_callbacks[i1] == NULL)
                {
                        _atexit_callbacks[i1] = pfn;
                        break;
                }
        }

        if ( i1 == 0 )//first handler
        {
                //to be called at normal process termination, either via
                //exit(3) or via return from the program¡¯s main()
                atexit(_call_atexit);

                int signos[] = {SIGHUP, SIGINT, SIGQUIT, SIGILL
                                        , SIGABRT, SIGBUS, SIGFPE
                                        , SIGTERM, SIGSEGV
                                        , SIGUSR1, SIGUSR2, SIGIO
                                        , SIGPIPE, SIGSYS};
                signal_ptr sig_pfn = NULL;
                
                for (i2 = 0; i2 < YF_ARRAY_SIZE(signos); ++i2)
                {
                        if (yf_replace_sig_handler(signos[i2], yf_exit_with_sig, &sig_pfn, log) != YF_OK)
                                continue;
                        
                        if (sig_pfn != SIG_DFL)
                        {
                                ret = yf_set_sig_handler(signos[i2], sig_pfn, log);
                                assert(ret == YF_OK);
                        }
                        else {
                                yf_log_debug(YF_LOG_DEBUG, log, 0, "catch sig=%d with atexit", signos[i2]);
                        }
                }
        }
        
        yf_unlock(&_atexit_lock);
        return i1 < YF_ARRAY_SIZE(_atexit_callbacks) ? YF_OK : YF_ERROR;
}

