#ifndef _YF_PROCESSOR_EVENT_IN_H_20120801_H
#define _YF_PROCESSOR_EVENT_IN_H_20120801_H
/*
* author: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20120801-20:09:51
*/

#include "yf_event_base_in.h"

struct yf_processor_event_in_s
{
        yf_int_t   proc_slot;

        yf_tm_evt_t *tm_evt;
        yf_fd_event_t *channel_read_evt;
        yf_fd_event_t *channel_write_evt;
        
        yf_processor_event_t  evt;

        yf_list_part_t  alloc_linker;
        yf_list_part_t  evt_linker;
};

struct  yf_proc_evt_driver_in_s
{
        yf_log_t *log;
        yf_list_part_t  proc_alloc_list, proc_evt_list;

        void  (*poll)(yf_proc_evt_driver_in_t* proc_driver);
};

yf_int_t   yf_init_proc_driver(yf_proc_evt_driver_in_t* proc_driver
                , yf_log_t* log);

yf_int_t   yf_destory_proc_driver(yf_proc_evt_driver_in_t* proc_driver);

#endif
