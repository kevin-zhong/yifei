#include "yf_event_base_in.h"

static yf_sig_driver_in_t  yf_sig_driver;

static void  yf_on_signal_in(int signo);
static void  yf_sig_poll(yf_sig_driver_in_t* sig_driver);

yf_sig_driver_in_t*  yf_init_sig_driver(yf_log_t* log)
{
        yf_memzero_st(yf_sig_driver);
        
        yf_sig_driver.log = log;
        yf_sig_driver.poll = yf_sig_poll;
        return  &yf_sig_driver;
}

yf_int_t   yf_reset_sig_driver(yf_log_t* log)
{
        yf_int_t i1 = 0;
        for ( i1 = 0; i1 < YF_ARRAY_SIZE(yf_sig_driver.singal_events); i1++ )
        {
                if (yf_sig_driver.singal_events[i1].set)
                {
                        yf_set_sig_handler(i1, SIG_IGN, log);
                }
        }
        yf_memzero_st(yf_sig_driver);
}

void  yf_on_signal_in(int signo)
{
        yf_sig_driver.siged_flag.bit_64 |= (1<< signo);
}


yf_int_t  yf_register_singal_evt(yf_evt_driver_t* driver
                , yf_signal_t* signal, yf_log_t* log)
{
        yf_int_t  signo = signal->signo;
        yf_sig_driver_in_t* sig_driver = ((yf_evt_driver_in_t*)driver)->sig_driver;
        
        if (signo >= YF_ARRAY_SIZE(sig_driver->singal_events))
        {
                yf_log_error(YF_LOG_ERR, log, 0, "invalid signo=%d", signo);
                return  YF_ERROR;
        }
        if (sig_driver->singal_events[signo].set)
        {
                yf_log_error(YF_LOG_ERR, log, 0, "sig already set, signo=%d", signo);
                return  YF_ERROR;
        }

        CHECK_RV(yf_set_sig_handler(signo, yf_on_signal_in, log), YF_ERROR);

        yf_log_debug2(YF_LOG_DEBUG, log, 0, "register sig evt, signo=%d, ptr=%P", 
                        signo, signal->sig_handler);

        sig_driver->singal_events[signo].signal = *signal;
        sig_driver->singal_events[signo].log = log;
        sig_driver->singal_events[signo].set = 1;
        return  YF_OK;
}


yf_int_t  yf_unregister_singal_evt(yf_evt_driver_t* driver
                , int  signo)
{
        yf_sig_driver_in_t* sig_driver = ((yf_evt_driver_in_t*)driver)->sig_driver;

        yf_sig_event_in_t* sig_evt = &sig_driver->singal_events[signo];
        if (!sig_evt->set)
        {
                yf_log_error(YF_LOG_ERR, sig_driver->log, 0, "sig not set, signo=%d", signo);
                return  YF_ERROR;
        }
        
        CHECK_RV(yf_set_sig_handler(signo, SIG_IGN, sig_evt->log), YF_ERROR);

        yf_log_debug2(YF_LOG_DEBUG, sig_evt->log, 0, "unregister sig evt, signo=%d, ptr=%P", 
                        signo, sig_evt->signal.sig_handler);
        
        yf_memzero(sig_evt, sizeof(yf_sig_event_in_t));
        return  YF_OK;
}


yf_int_t  yf_register_singal_evts(yf_evt_driver_t* driver
                , yf_signal_t* signals, yf_log_t* log)
{
        yf_signal_t* sig;
        
        for (sig = signals; sig->signo != 0; sig++)
        {
                CHECK_RV(yf_register_singal_evt(driver, sig, log) != YF_OK, YF_ERROR);
        }
        return YF_OK;
}


void  yf_sig_poll(yf_sig_driver_in_t* sig_driver)
{
        yf_int_t i1 = 0;
        yf_int_t sig_no;
        yf_set_bits  set_bits;
        signal_ptr  sig_handler;
        
        yf_get_set_bits(&sig_driver->siged_flag, set_bits);

        for ( i1 = 0; i1 < YF_BITS_CNT; i1++ )
        {
                sig_no = set_bits[i1];
                if (sig_no == YF_END_INDEX)
                        break;

                sig_handler = sig_driver->singal_events[sig_no].signal.sig_handler;
                if (sig_handler)
                {
                        yf_log_debug1(YF_LOG_DEBUG, sig_driver->singal_events[sig_no].log, 0, 
                                        "sig=[%d] catched", sig_no);
                        sig_handler(sig_no);
                }
                else {
                        yf_log_error(YF_LOG_WARN, sig_driver->log, 0, 
                                        "sig=[%d] catched, but no handler !", sig_no);
                }
        }
        sig_driver->siged_flag.bit_64 = 0;
}

