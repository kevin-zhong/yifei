#ifndef _YF_SIG_EVENT_IN_H_20120801_H
#define _YF_SIG_EVENT_IN_H_20120801_H
/*
* copyright@: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20120801-21:39:26
*/

typedef struct yf_sig_event_in_s
{
        yf_int_t  set;
        yf_sig_event_t evt;
}
yf_sig_event_in_t;


//max sig no = 64
struct yf_sig_driver_in_s
{
        yf_bit_set_t  siged_flag;
        
        yf_sig_event_in_t  singal_events[YF_BITS_CNT];

        yf_log_t *log;

        void  (*poll)(yf_sig_driver_in_t* sig_driver);
};

yf_sig_driver_in_t*   yf_init_sig_driver(yf_log_t* log);
yf_int_t   yf_reset_sig_driver(yf_log_t* log);

#endif
