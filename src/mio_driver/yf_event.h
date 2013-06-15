#ifndef  _YF_EVENT_H
#define _YF_EVENT_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

#define yf_evt_driver_t yf_u64_t

#define YF_EVT_DATA union { \
                yf_u64_t  data2; \
                yf_u32_t  data3[2]; \
                void* data4; \
        }

typedef  struct  yf_tm_evt_s
{
        void*        data;
        YF_EVT_DATA;
        yf_log_t*   log;

        yf_evt_driver_t*  driver;

        void (*timeout_handler)(struct yf_tm_evt_s* evt, yf_time_t* start);
} 
yf_tm_evt_t;

#define  YF_INVALID_FD -1

#define  YF_REVT  0x01
#define  YF_WEVT 0x02

extern const yf_str_t yf_evt_type_n[];


typedef  struct yf_fd_event_s
{
        yf_fd_t        fd;
        
        yf_u32_t   type:2;
        yf_u32_t   ready:1;
        yf_u32_t   eof:1;//if recv, check eof+error
        yf_u32_t   timeout:1;//just in fd_evt_handle check this
        yf_u32_t   error:1;
        yf_u32_t   shutdown:1;//if send+recv, check shutdown+error
        
        void*        data;
        YF_EVT_DATA;
        yf_log_t*   log;

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

#define YF_DEFAULT_DRIVER_CB NULL, NULL, NULL, NULL, NULL


yf_evt_driver_t*  yf_evt_driver_create(yf_evt_driver_init_t* driver_init);
void yf_evt_driver_destory(yf_evt_driver_t* driver);

yf_evt_driver_init_t* yf_evt_driver_ctx(yf_evt_driver_t* driver);

void yf_evt_driver_stop(yf_evt_driver_t* driver);
void yf_evt_driver_start(yf_evt_driver_t* driver);


yf_int_t  yf_alloc_fd_evt(yf_evt_driver_t* driver, yf_fd_t fd
                , yf_fd_event_t** read, yf_fd_event_t** write, yf_log_t* log);
//pwrite can be set=NULL
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

/*
* just after alloc, then you can use this func
*/
yf_fd_event_t* yf_get_fd_evt(yf_evt_driver_t* driver, yf_fd_t fd, yf_u32_t type);


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
yf_int_t  yf_free_tm_evt(yf_tm_evt_t* tm_evt);

yf_int_t   yf_register_tm_evt(yf_tm_evt_t* tm_evt, yf_time_t  *time_out);
yf_int_t   yf_unregister_tm_evt(yf_tm_evt_t* tm_evt);

/*
* singal evt
* in one proc, just the main thread can regist signal evt...
* no need to alloc before regist
*/
typedef struct yf_sig_event_s
{
        int            signo;
        
        void*        data;
        YF_EVT_DATA;
        yf_log_t*   log;

        yf_evt_driver_t*  driver;

        void (*sig_evt_handler)(struct yf_sig_event_s* evt);
}
yf_sig_event_t;

yf_int_t  yf_register_singal_evt(yf_evt_driver_t* driver
                , yf_sig_event_t* sig_evt, yf_log_t* log);

yf_int_t  yf_unregister_singal_evt(yf_evt_driver_t* driver
                , int  signo);

/*
* processor evt, just run once
*/
typedef struct yf_processor_event_s
{
        yf_exec_ctx_t exec_ctx;
        
        //if no need to read, pool can be set with NULL
        yf_pool_t    *pool;
        yf_chain_t  *write_chain;
        yf_chain_t  *read_chain;

        yf_u16_t   processing:1;
        yf_u16_t   timeout:1;
        yf_u16_t   error:1;
        yf_u16_t   exit_code:8;

        void*        data;
        YF_EVT_DATA;
        yf_log_t*   log;

        yf_evt_driver_t* driver;
        
        void (*ret_handler)(struct yf_processor_event_s* evt);
}
yf_processor_event_t;

yf_int_t  yf_alloc_proc_evt(yf_evt_driver_t* driver, yf_processor_event_t** proc_evt
                , yf_log_t* log);
yf_int_t  yf_free_proc_evt(yf_processor_event_t* proc_evt);

yf_int_t   yf_register_proc_evt(yf_processor_event_t* proc_evt, yf_time_t  *time_out);
yf_int_t   yf_unregister_proc_evt(yf_processor_event_t* proc_evt);

yf_process_t* yf_get_proc_by_evt(yf_processor_event_t* proc_evt);

#endif

