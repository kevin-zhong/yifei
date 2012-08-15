#ifndef _YF_BRIDGE_TASK_H_20120812_H
#define _YF_BRIDGE_TASK_H_20120812_H
/*
* author: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20120812-13:52:16
*/

#include "yf_bridge_in.h"

typedef struct task_info_s
{
        yf_u64_t  id;
        yf_time_t  inqueue_time;
        yf_int_t   status;
        yf_u32_t  timeout_ms:20;
        yf_u32_t  timeout:1;
        yf_u32_t  error:1;
}
task_info_t;

typedef struct yf_task_queue_s
{
        yf_u32_t magic;
        size_t  capacity;

        /*
        * note, if read_offset == write_offset, then tq is empty to read
        * the free size must > 0 (else cant identify empty with full)
        */
        volatile size_t  read_offset;
        volatile size_t  write_offset;
}
yf_task_queue_t;


yf_task_queue_t*  yf_init_task_queue(char* buf, size_t size, yf_log_t* log);

yf_int_t  yf_task_push(yf_task_queue_t* queue, task_info_t* task_info
                , char* task, size_t task_len, yf_log_t* log);

//if empty, will return YF_AGAIN...
yf_int_t  yf_task_pop(yf_task_queue_t* queue, task_info_t* task_info
                , char* task, size_t* task_len, yf_log_t* log);

#define yf_tq_free_capacity(tq) (((tq)->read_offset + (tq)->capacity - (tq)->write_offset) \
                % (tq)->capacity)
#define yf_tq_buf_size(tq) (((tq)->write_offset + (tq)->capacity - (tq)->read_offset) \
                % (tq)->capacity)

#endif
