#ifndef  _YF_EVENT_IN_H
#define _YF_EVENT_IN_H

#include "yf_event_base_in.h"

#define YF_INVALID_INDEX  0xd0d0d0d0

struct  yf_fd_evt_link_s
{
        yf_fd_event_t  evt;

        yf_list_part_t  active_linker;
        yf_list_part_t  ready_linker;

        yf_uint_t   index;

        yf_u16_t  timeset:1;
        yf_u16_t  active:1;
        yf_u16_t  polled:1;//may not active, but still in poll
        
        yf_u32_t  last_active_op_index;

        yf_timer_t   timer;
};


struct yf_fd_evt_in_s
{
        yf_s32_t         use_flag;
        yf_list_part_t  link;
        yf_fd_evt_link_t  read;
        yf_fd_evt_link_t  write;
};


struct  yf_fd_evt_driver_in_s
{
        yf_fd_poll_t*  evt_poll;

        yf_list_part_t      all_evt_list;
        
        yf_list_part_t      ready_list   ____cacheline_aligned;

        yf_u32_t            active_op_index;

        yf_u32_t            evts_capcity;
        yf_u32_t            evts_num;
        yf_fd_evt_in_t   *pevents;
        
}  ____cacheline_aligned;


yf_int_t   yf_init_fd_driver(yf_fd_evt_driver_in_t* fd_driver
                , yf_u32_t nfds, yf_s32_t  poll_type, yf_log_t* log);

void  yf_destory_fd_driver(yf_fd_evt_driver_in_t *fd_driver);

#endif

