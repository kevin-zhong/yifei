#ifndef _YF_BRIDGE_H_20120811_H
#define _YF_BRIDGE_H_20120811_H
/*
* author: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20120811-16:44:28
*/

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>
#include <mio_driver/yf_event.h>

#define YF_BRIDGE_EVT_DRIVED 0
#define YF_BRIDGE_BLOCKED 1

#define YF_BRIDGE_INS_PROC 0
#define YF_BRIDGE_INS_THREAD 1

#define YF_TASK_DISTPATCH_HASH_MOD 1
#define YF_TASK_DISTPATCH_IDLE 2

#define YF_TASK_SUCESS 0
#define YF_TASK_TIMEOUT 1
#define YF_TASK_ERROR 2

#define YF_BRIDGE_MAX_CHILD_NUM 32

typedef  yf_u64_t yf_bridge_t;

/*
* will copy content from bridge to the mem that first ptr pointing
* if task_len != NULL, then will be the input buf size, if input buf size
* small than taskuired, then will return YF_ERROR; after call, the res
* len is returned with task_len..
* else if task_len == NULL, then will asume that buf is big enough, and
* the res len is known to the caller...
* after get, will delete the task.. from bridge
*/
typedef void (*yf_task_handle)(yf_bridge_t* bridge
                , void* task, size_t len, yf_u64_t id, yf_log_t* log);

typedef void (*yf_task_res_handle)(yf_bridge_t* bridge
                , void* task_res, size_t len, yf_u64_t id
                , yf_int_t status, void* data, yf_log_t* log);

typedef struct yf_bridge_cxt_s
{
        yf_uint_t  parent_ins_type:2;
        yf_uint_t  child_ins_type:2;
        yf_uint_t  parent_poll_type:2;
        yf_uint_t  child_poll_type:2;
        yf_uint_t  task_dispatch_type:3;

        void* exec_func;
        
        //max = YF_BRIDGE_MAX_CHILD_NUM
        yf_uint_t child_num;

        //note, must > real max task size + 64byte
        size_t max_task_size;
        size_t max_task_num;
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
yf_bridge_t* yf_bridge_create(yf_bridge_cxt_t* bridge_ctx, yf_log_t* log);

//will destory childs and destory bridge...
yf_int_t  yf_bridge_destory(yf_bridge_t* bridge, yf_log_t* log);

yf_int_t yf_attach_res_bridge(yf_bridge_t* bridge
                , yf_evt_driver_t* evt_driver, yf_task_res_handle handler, yf_log_t* log);

//will return taskid>0 if success, else ret -1
//if timeout_ms = 0, then will wait forever...
yf_u64_t yf_send_task(yf_bridge_t* bridge
                , void* task, size_t len, yf_u32_t hash
                , void* data, yf_u32_t timeout_ms, yf_log_t* log);

//if in block type, call this will block, else will ret quickly with no effects
void yf_poll_task_res(yf_bridge_t* bridge, yf_log_t* log);


/*
* ------child's api------
*/
//child call this after born and init
//if in block type, set evt_driver=NULL
yf_int_t yf_attach_bridge(yf_bridge_t* bridge
                , yf_evt_driver_t* evt_driver, yf_task_handle handler, yf_log_t* log);

//if in block type, call this will block, else will ret quickly with no effects
void yf_poll_task(yf_bridge_t* bridge, yf_log_t* log);

//if len==0, then no res body
yf_int_t yf_send_task_res(yf_bridge_t* bridge
                , void* task_res, size_t len, yf_u64_t id
                , yf_int_t status, yf_log_t* log);


#endif