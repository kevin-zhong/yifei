#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include "yf_bridge_in.h"

#define yf_tq_head_len yf_align(sizeof(yf_task_queue_t), YF_ALIGNMENT)
#define yf_tq_buf(tq) ((char*)(tq) + yf_tq_head_len)

static void yf_tq_buf_add(yf_task_queue_t* queue, size_t* write_off
                , char* buf, size_t len, yf_log_t* log);

static yf_int_t yf_tq_buf_get(yf_task_queue_t* queue, size_t* read_off
                , char* buf, size_t len, yf_log_t* log);

typedef struct yf_task_head_s
{
        yf_u32_t  magic;
        yf_u32_t  task_len;
        yf_u32_t  inqueue_time;

        task_info_t task_info;
}
yf_task_head_t;


yf_task_queue_t*  yf_init_task_queue(char* buf, size_t size, yf_log_t* log)
{
        yf_task_queue_t* queue = (yf_task_queue_t*)buf;

        if (yf_check_magic(queue->magic))
        {
                yf_log_debug3(YF_LOG_DEBUG, log, 0, 
                                "task queue already inited, capacity=%d, r=%d, w=%d", 
                                queue->capacity, queue->read_offset, queue->write_offset);
                return queue;
        }
        if (size <= yf_tq_head_len)
        {
                yf_log_error(YF_LOG_ERR, log, 0, "too small buf, size=%d", size);
                return queue;
        }

        yf_memzero(buf, yf_tq_head_len);
        yf_set_magic(queue->magic);
        queue->capacity = size - yf_tq_head_len;
        
        return queue;
}


yf_int_t  yf_task_push(yf_task_queue_t* queue, task_info_t* task_info
                , char* task, size_t task_len, yf_log_t* log)
{
        assert(yf_check_magic(queue->magic));
        
        size_t free_cap = yf_tq_free_capacity(queue);
        size_t require_len = task_len + sizeof(yf_task_head_t);
        size_t write_off;

        //note, must > , then can push sucess...
        if (unlikely(free_cap <= require_len))
        {
                yf_log_error(YF_LOG_WARN, log, 0, "tq free_cap=%d, require_len=%d, push fail", 
                                free_cap, require_len);
                return  YF_ERROR;
        }

        yf_task_head_t task_head = {YF_MAGIC_VAL, task_len, 
                        yf_now_times.clock_time.tv_sec, *task_info};

        write_off = queue->write_offset;
        
        yf_tq_buf_add(queue, &write_off, (char*)&task_head, sizeof(yf_task_head_t), log);
        if (task_len)
                yf_tq_buf_add(queue, &write_off, task, task_len, log);

        queue->write_offset = write_off;

        yf_log_debug5(YF_LOG_DEBUG, log, 0, 
                        "tq cap=%d, free=%d, after task push, r=%d, w=%d, tlen=%d", 
                        queue->capacity, yf_tq_free_capacity(queue), 
                        queue->read_offset, queue->write_offset, 
                        task_len);
        
        return YF_OK;
}


yf_int_t  yf_task_pop(yf_task_queue_t* queue, task_info_t* task_info
                , char* task, size_t* task_len, yf_log_t* log)
{
        assert(yf_check_magic(queue->magic));
        
        yf_task_head_t  task_head;
        size_t read_off = queue->read_offset;
        
        CHECK_OK(yf_tq_buf_get(queue, &read_off, 
                        (char*)&task_head, sizeof(yf_task_head_t), log));
        assert(yf_check_magic(task_head.magic));

        if (unlikely(task_len && *task_len < task_head.task_len))
        {
                yf_log_error(YF_LOG_WARN, log, 0, "res buf size=%d not big enough<%d", 
                                *task_len, task_head.task_len);
                return YF_ERROR;
        }

        if (task_head.task_len) {
                CHECK_OK(yf_tq_buf_get(queue, &read_off, 
                        task, task_head.task_len, log));
        }
        
        queue->read_offset = read_off;

        if (task_info)
                *task_info = task_head.task_info;
        if (task_len)
                *task_len = task_head.task_len;

        yf_log_debug3(YF_LOG_DEBUG, log, 0, "tq cap=%d, after task pop, r=%d, w=%d", 
                        queue->capacity, queue->read_offset, queue->write_offset);
        return YF_OK;
}


void yf_tq_buf_add(yf_task_queue_t* queue, size_t* write_off
                , char* buf, size_t len, yf_log_t* log)
{
        size_t tail_cap = queue->capacity - *write_off;
        ssize_t rest_len = len - tail_cap;
        
        char* tq_buf = yf_tq_buf(queue);

        yf_memcpy(tq_buf + *write_off, buf, rest_len > 0 ? tail_cap : len);
        
        /*yf_log_debug2(YF_LOG_DEBUG, log, 0, "write to %d len=%d", 
                        *write_off, rest_len > 0 ? tail_cap : len);*/
        
        if (rest_len < 0)
        {
                *write_off += len;
        }
        else if (rest_len == 0) {
                *write_off = 0;
        }
        else {
                //yf_log_debug1(YF_LOG_DEBUG, log, 0, "write to 0 len=%d", rest_len);
                
                yf_memcpy(tq_buf, buf + tail_cap, rest_len);
                *write_off = rest_len;
        }
}


yf_int_t yf_tq_buf_get(yf_task_queue_t* queue, size_t* read_off
                , char* buf, size_t len, yf_log_t* log)
{
        size_t buf_size = ((queue)->write_offset + (queue)->capacity - *read_off) \
                        % (queue)->capacity;
        
        if (buf_size < len)
        {
                yf_log_debug2(YF_LOG_DEBUG, log, 0, "tq buf size=%d < ask len=%d", 
                                buf_size, len);
                return YF_AGAIN;
        }

        size_t tail_cap = queue->capacity - *read_off;
        ssize_t rest_len = len - tail_cap;
        
        char* tq_buf = yf_tq_buf(queue);

        yf_memcpy(buf, tq_buf + *read_off, rest_len > 0 ? tail_cap : len);
        
        /*yf_log_debug2(YF_LOG_DEBUG, log, 0, "read from %d len=%d", 
                        *read_off, rest_len > 0 ? tail_cap : len);*/
        
        if (rest_len < 0)
        {
                *read_off += len;
        }
        else if (rest_len == 0) {
                *read_off = 0;
        }
        else {
                //yf_log_debug1(YF_LOG_DEBUG, log, 0, "read from 0 len=%d", rest_len);
                
                yf_memcpy(buf + tail_cap, tq_buf, rest_len);
                *read_off = rest_len;
        }

        return YF_OK;
}
