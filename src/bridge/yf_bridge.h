#ifndef _YF_BRIDGE_H_20120811_H
#define _YF_BRIDGE_H_20120811_H
/*
* author: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20120811-16:44:28
*/

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>
#include <mio_driver/yf_event.h>

#define YF_BRIDGE_BLOCKED 1
#define YF_BRIDGE_EVT_DRIVED 2

#define YF_BRIDGE_INS_PROC 1
#define YF_BRIDGE_INS_THREAD 2

#define YF_TASK_DISTPATCH_HASH_MOD 1
#define YF_TASK_DISTPATCH_HASH_AREA 2
#define YF_TASK_DISTPATCH_IDLE 3

#define YF_TASK_SUCESS 0
#define YF_TASK_TIMEOUT 1
#define YF_TASK_ERROR 2

typedef  yf_u64_t yf_bridge_t;

typedef void (*yf_task_handle)(yf_bridge_t* bridge
                , void* task, size_t* len, yf_u32_t id);

typedef void (*yf_task_res_handle)(yf_bridge_t* bridge
                , void* task_res, size_t* len, yf_u32_t id
                , yf_int_t status, void* data);

typedef struct yf_bridge_cxt_s
{
        yf_uint_t  parent_ins_type:2;
        yf_uint_t  child_ins_type:2;
        yf_uint_t  parent_poll_type:2;
        yf_uint_t  child_poll_type:2;
        yf_uint_t  task_dispatch_type:3;

        void* exec_func;
        yf_uint_t child_num;
        
        size_t queue_capacity;
        
        yf_log_t* log;
}
yf_bridge_cxt_t;


yf_uint_t  yf_bridge_task_num(yf_bridge_t* bridge);


/*
* ------parent's api------
*/
//if dispatch_func == NULL, then the idle child will deal the task, else
//the dispated target child will deal the task
yf_bridge_t* yf_bridge_create(yf_bridge_cxt_t bridge_ctx
                , yf_evt_driver_t* evt_driver, yf_task_res_handle task_res_handler);

//will destory childs and destory bridge...
yf_int_t  yf_bridge_destory(yf_bridge_t* bridge);

//will return taskid>0 if success, else ret -1
yf_u32_t yf_send_task(yf_bridge_t* bridge
                , void* task, size_t len, yf_u32_t hash
                , void* data, yf_log_t* log);

//if in block type, call this will block, else will ret quickly with no effects
void yf_poll_task_res(yf_bridge_t* bridge);


/*
* ------child's api------
*/
//child call this after born and init
//if in block type, set evt_driver=NULL
yf_int_t yf_attach_bridge(yf_bridge_t* bridge
                , yf_evt_driver_t* evt_driver, yf_task_handle task_handler);

//if in block type, call this will block, else will ret quickly with no effects
void yf_poll_task(yf_bridge_t* bridge);

//if task_res==NULLL or len==0, then no res body
yf_int_t yf_send_task_res(yf_bridge_t* bridge
                , void* task_res, size_t len, yf_u32_t id
                , yf_int_t status, yf_log_t* log);


#endif
