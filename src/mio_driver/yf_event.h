#ifndef  _YF_EVENT_H
#define _YF_EVENT_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

typedef  struct  yf_tm_evt_s
{
        void*        data;
        yf_log_t*   log;

        void (*timeout_handler)(struct yf_tm_evt_s* evt, yf_time_t* start);
} 
yf_tm_evt_t;

#define  YF_INVALID_FD -1

#define  YF_REVT  0x01
#define  YF_WEVT 0x02

extern const yf_str_t yf_evt_type_n[];

#define yf_evt_driver_t int


typedef  struct yf_fd_event_s
{
        yf_fd_t        fd;
        
        yf_u32_t   type:2;
        yf_u32_t   ready:1;
        yf_u32_t   eof:1;
        yf_u32_t   timeout:1;
        yf_u32_t   error:1;
        
        void*           data;
        yf_log_t*     log;

        yf_evt_driver_t*  driver;

        void (*fd_evt_handler)(struct yf_fd_event_s* evt);
} 
yf_fd_event_t;

#define yf_evt_tn(pev) yf_evt_type_n[(pev)->type]


#define  YF_POLL_BY_DETECT 0
#define  YF_POLL_BY_SELECT 1
#define  YF_POLL_BY_POLL     2
#define  YF_POLL_BY_EPOLL   3

typedef  struct
{
        yf_s32_t  poll_type;
        yf_u32_t nfds;
        yf_u32_t nstimers;

        yf_log_t*  log;

        void (*start_cb)(yf_evt_driver_t*, void*, yf_log_t* );
        void (*poll_cb)(yf_evt_driver_t*, void*, yf_log_t* );
        void (*stop_cb)(yf_evt_driver_t*, void*, yf_log_t* );
        void (*destory_cb)(yf_evt_driver_t*, void*, yf_log_t* );
        void *data;
}
yf_evt_driver_init_t;


yf_evt_driver_t*  yf_evt_driver_create(yf_evt_driver_init_t* driver_init);
void yf_evt_driver_destory(yf_evt_driver_t* driver);

void yf_evt_driver_stop(yf_evt_driver_t* driver);
void yf_evt_driver_start(yf_evt_driver_t* driver);


yf_int_t  yf_alloc_fd_evt(yf_evt_driver_t* driver, yf_fd_t fd
                , yf_fd_event_t** read, yf_fd_event_t** write, yf_log_t* log);
yf_int_t  yf_free_fd_evt(yf_fd_event_t* pread
                , yf_fd_event_t* pwrite);

/*
*oneshot event handler, means one register just call handle once...
*/
yf_int_t  yf_register_fd_evt(yf_fd_event_t* pevent, yf_time_t  *time_out);
/*
*unregister used before event happen, means interupt fd poll
*/
yf_int_t  yf_unregister_fd_evt(yf_fd_event_t* pevent);


#define FD_TIMER_NEW 0x01
#define FD_TIMER_DEL  0x02
/*
* timer_new : fi org event have timer, then will cancel org first, 
*                     then add new with 0 passed time;
* timer_del   : cancell org
*/
yf_int_t  yf_fd_evt_timer_ctl(yf_fd_event_t* pevent, int mode, yf_time_t* time_out);


yf_int_t  yf_alloc_tm_evt(yf_evt_driver_t* driver, yf_tm_evt_t** tm_evt
                , yf_log_t* log);
yf_int_t  yf_free_tm_evt(yf_evt_driver_t* driver, yf_tm_evt_t* tm_evt);

yf_int_t   yf_register_tm_evt(yf_evt_driver_t* driver, yf_tm_evt_t* tm_evt
                , yf_time_t  *time_out);
yf_int_t   yf_unregister_tm_evt(yf_evt_driver_t* driver, yf_tm_evt_t* tm_evt);

#endif

