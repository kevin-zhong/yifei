#ifndef  _YF_POLL_H
#define _YF_POLL_H

#include "yf_event_base_in.h"

struct  yf_fd_poll_actions_s
{
        yf_fd_poll_t* (*init)(yf_u32_t nfds);
        yf_int_t (*uninit)(yf_fd_poll_t* poller);
        
        yf_int_t (*add)(yf_fd_poll_t* poller, yf_fd_evt_in_t* fd_evt);
        yf_int_t (*del)(yf_fd_poll_t* poller, yf_fd_evt_in_t* fd_evt);
        
        yf_int_t (*activate)(yf_fd_poll_t* poller, yf_fd_evt_link_t* evt);
        yf_int_t (*deactivate)(yf_fd_poll_t* poller, yf_fd_evt_link_t* evt);

        yf_int_t (*dispatch)(yf_fd_poll_t* poller);
};

typedef  struct
{
        const char* name;
        yf_fd_poll_actions_t  actions;
}
yf_fd_poll_cls_t;

/*
* poll interface
*/
struct yf_fd_poll_s
{
        yf_fd_evt_driver_in_t* ctx;
        void* agen_data;
        yf_log_t* log;

        yf_fd_poll_cls_t *poll_cls;
};

extern yf_fd_poll_cls_t  yf_fd_poller_cls[];
extern yf_uint_t  yf_fd_poller_cls_num;

void  yf_load_fd_pollers();

#endif

