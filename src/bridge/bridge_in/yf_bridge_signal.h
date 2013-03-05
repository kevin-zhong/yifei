#ifndef _YF_BRIDGE_SIGNAL_H_20120816_H
#define _YF_BRIDGE_SIGNAL_H_20120816_H
/*
* copyright@: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20120816-19:21:47
*/

#include "yf_bridge_in.h"

/*
* channel
*/
typedef struct yf_bridge_channel_s
{
        yf_socket_t  channels[2];
        yf_u32_t  owner:1;
        yf_u32_t  pblocked:1;
        yf_u32_t  cblocked:1;
        yf_u32_t  child_no:16;
        yf_fd_event_t*  channel_pevts[2];
        yf_fd_event_t*  channel_cevts[2];
        yf_bridge_in_t* bridge_in;
}
yf_bridge_channel_t;

yf_int_t  yf_bridge_channel_init(yf_bridge_channel_t* bc
                , yf_bridge_in_t* bridge
                , yf_socket_t* channels, yf_evt_driver_t* evt_driver
                , yf_int_t is_parent, yf_int_t child_no, yf_log_t* log);

yf_int_t  yf_bridge_channel_uninit(yf_bridge_channel_t* bc
                , yf_int_t is_parent, yf_log_t* log);

yf_int_t  yf_bridge_channel_signal(yf_bridge_channel_t* bc
                , yf_int_t is_parent, yf_log_t* log);

yf_int_t  yf_bridge_channel_wait(yf_bridge_channel_t* bc
                , yf_int_t is_parent, yf_log_t* log);

/*
* mutex+cond
*/
//just for thread + thread
typedef struct yf_bridge_mutex_s
{
        yf_uint_t  wait_num;
        yf_mutex_t* mutex;
        yf_cond_t*   cond;
        yf_bridge_in_t* bridge_in;
}
yf_bridge_mutex_t;

yf_int_t  yf_bridge_mutex_init(yf_bridge_mutex_t* bm
                , yf_bridge_in_t* bridge, yf_log_t* log);

yf_int_t  yf_bridge_mutex_uninit(yf_bridge_mutex_t* bm
                , yf_log_t* log);

yf_int_t  yf_bridge_mutex_lock(yf_bridge_mutex_t* bm
                , yf_log_t* log);
yf_int_t  yf_bridge_mutex_unlock(yf_bridge_mutex_t* bm
                , yf_log_t* log);

yf_int_t  yf_bridge_mutex_signal(yf_bridge_mutex_t* bm, yf_int_t alreay_locked
                , yf_log_t* log);

yf_int_t  yf_bridge_mutex_wait(yf_bridge_mutex_t* bm
                , yf_log_t* log);

#endif
