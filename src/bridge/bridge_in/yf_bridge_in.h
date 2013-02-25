#ifndef _YF_BRIDGE_IN_H_20120811_H
#define _YF_BRIDGE_IN_H_20120811_H
/*
* author: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20120811-18:07:44
*/

#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <bridge/yf_bridge.h>

#include "yf_bridge_task.h"

#define  YF_ANY_CHILD_NO 0xfffe


typedef struct yf_bridge_in_s
{
        yf_u32_t  begin_flag;
        
        yf_bridge_cxt_t  ctx;
        yf_node_pool_t  task_cb_info_pool;

        void* bridge_data;

        char* task_res_buf;
        char* task_buf[YF_BRIDGE_MAX_CHILD_NUM];
        yf_uint_t  task_execut[YF_BRIDGE_MAX_CHILD_NUM];

        //all can call
        //may change child_no if arg is ptr...
        yf_int_t (*lock_tq)(struct yf_bridge_in_s* bridge
                        , yf_task_queue_t** tq, yf_int_t* child_no, yf_log_t* log);
        void (*unlock_tq)(struct yf_bridge_in_s* bridge
                        , yf_task_queue_t* tq, yf_int_t child_no, yf_log_t* log);

        yf_int_t (*lock_res_tq)(struct yf_bridge_in_s* bridge
                        , yf_task_queue_t** tq, yf_int_t* child_no, yf_log_t* log);
        void (*unlock_res_tq)(struct yf_bridge_in_s* bridge
                        , yf_task_queue_t* tq, yf_int_t child_no, yf_log_t* log);        

        /*
        * parent call
        */
        yf_task_res_handle task_res_handler;

        yf_evt_driver_t* parent_evt_driver;

        //yes, task_signal ret void, cause if fail, child may exit...
        //and, you should lock tq before call this...
        void (*task_signal)(struct yf_bridge_in_s* bridge
                        , yf_task_queue_t* tq, yf_int_t child_no, yf_log_t* log);

        yf_int_t (*attach_res_bridge)(struct yf_bridge_in_s* bridge
                        , yf_evt_driver_t* evt_driver, yf_log_t* log);

        void (*wait_task_res)(struct yf_bridge_in_s* bridge
                        , yf_int_t* child_no, yf_log_t* log);

        void (*destory)(struct yf_bridge_in_s* bridge, yf_log_t* log);

        /*
        * child call
        */
        yf_task_handle task_handler;

        yf_int_t (*child_no)(struct yf_bridge_in_s* bridge
                        , yf_log_t* log);
        yf_int_t (*attach_child_ins)(struct yf_bridge_in_s* bridge
                        , yf_log_t* log);

        void (*task_res_signal)(struct yf_bridge_in_s* bridge
                        , yf_task_queue_t* tq, yf_int_t child_no, yf_log_t* log);

        yf_int_t (*attach_bridge)(struct yf_bridge_in_s* bridge
                        , yf_evt_driver_t* evt_driver, yf_int_t child_no, yf_log_t* log);
        
        void (*wait_task)(struct yf_bridge_in_s* bridge
                        , yf_int_t child_no, yf_log_t* log);

        yf_u32_t  end_flag;
}
yf_bridge_in_t;


typedef struct yf_task_ctx_s
{
        void* data;
        yf_tm_evt_t* tm_evt;
        yf_bridge_in_t* bridge_in;
        yf_int_t  child_no;
}
yf_task_ctx_t;


#define __yf_bridge_set_ac(brige, ac, prefix) (brige)->ac = prefix ## _ ## ac;


yf_int_t yf_send_task_res_in(yf_bridge_in_t* bridge_in
                , void* task_res, size_t len, task_info_t* task_info, yf_log_t* log);

void  yf_bridge_on_task_valiable(yf_bridge_in_t* bridge_in
                , yf_int_t child_no, yf_log_t* log);

void  yf_bridge_on_task_res_valiable(yf_bridge_in_t* bridge_in
                , yf_int_t child_no, yf_log_t* log);

yf_int_t  yf_bridge_get_idle_tq(yf_bridge_in_t* bridge_in);

#include "yf_bridge_signal.h"


#endif
