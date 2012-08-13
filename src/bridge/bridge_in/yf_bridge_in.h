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

typedef struct yf_bridge_in_s
{
        yf_bridge_cxt_t  ctx;

        void* bridge_data;

        yf_int_t (*send_task)(yf_bridge_t* bridge
                , void* task, size_t len, yf_u32_t hash
                , void* data, yf_log_t* log);
        /*
        * will copy content from bridge to the mem that first ptr pointing
        * if task_len != NULL, then will be the input buf size, if input buf size
        * small than taskuired, then will return YF_ERROR; after call, the res
        * len is returned with task_len..
        * else if task_len == NULL, then will asume that buf is big enough, and
        * the res len is known to the caller...
        * after get, will delete the task.. from bridge
        */
        yf_int_t (*get_task)(yf_bridge_t* bridge
                , void* task, size_t* len, yf_log_t* log);

        //child's api
        yf_int_t (*send_task_res)(yf_bridge_t* bridge
                , void* task_res, size_t len, yf_log_t* log);

        yf_int_t (*get_task_res)(yf_bridge_t* bridge
                , void* task_res, size_t* len, yf_log_t* log);
        
}
yf_bridge_in_t;


#endif
