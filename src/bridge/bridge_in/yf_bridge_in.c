#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include "yf_bridge_in.h"

/*
* ------parent's api------
*/
//if dispatch_func == NULL, then the idle child will deal the task, else
//the dispated target child will deal the task
yf_bridge_t* yf_bridge_create(yf_bridge_cxt_t* bridge_ctx
                , yf_evt_driver_t* evt_driver, yf_task_res_handle handler, yf_log_t* log)
{
        yf_bridge_in_t* bridge_in;
        
        bridge_in->task_res_handler = handler;

        //last call this
        bridge_in->ctx = *bridge_ctx;
        
        if (bridge_ctx->max_task_size >= (bridge_ctx->queue_capacity>>2))
        {
                yf_log_error(YF_LOG_DEBUG, log, 0, 
                        "max_task_size val=%d not right to queue cap=%d, reset to %d",
                        bridge_ctx->max_task_size, 
                        bridge_ctx->queue_capacity, 
                        bridge_ctx->queue_capacity>>2);
                
                bridge_ctx->max_task_size = bridge_ctx->queue_capacity>>2;
        }
        
        bridge_in->task_res_buf = yf_alloc(bridge_ctx->max_task_size);
        
        return  (yf_bridge_t*)bridge_in;
}


//will destory childs and destory bridge...
yf_int_t  yf_bridge_destory(yf_bridge_t* bridge, yf_log_t* log)
{
        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)bridge;
        assert(yf_check_be_magic(bridge_in));

        yf_free(bridge_in->task_res_buf);
        
        return YF_OK;
}


//will return taskid>0 if success, else ret -1
yf_u64_t yf_send_task(yf_bridge_t* bridge
                , void* task, size_t len, yf_u32_t hash
                , void* data, yf_u32_t timeout_ms, yf_log_t* log)
{
        yf_task_cb_info_t* task_cb;
        task_info_t  task_info;
        yf_u64_t  task_id;
        yf_int_t  ret;
        yf_int_t  child_no;
        yf_task_queue_t* tq = NULL;
        
        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)bridge;
        assert(yf_check_be_magic(bridge_in));

        if (len > bridge_in->ctx.max_task_size)
        {
                yf_log_error(YF_LOG_ERR, log, 0, "task len=%d too big > max_task_size=%d", 
                                len, bridge_in->ctx.max_task_size);
                return -1;
        }

        //get child_no
        switch (bridge_in->ctx.task_dispatch_type)
        {
                case YF_TASK_DISTPATCH_HASH_MOD:
                        child_no = hash % bridge_in->ctx.child_num;
                        break;
                default:
                        child_no = YF_ANY_CHILD_NO;//default
                        break;
        }

        task_cb = yf_alloc_node_from_pool(&bridge_in->task_cb_info_pool, log);
        if (task_cb == NULL)
        {
                yf_log_error(YF_LOG_WARN, log, 0, "alloc task cb failed");
                return -1;
        }
        task_cb->data = data;
        
        task_id = yf_get_id_by_node(&bridge_in->task_cb_info_pool, task_cb, log);
        
        ret = bridge_in->lock_tq(bridge_in, &tq, &child_no, log);
        if (ret != YF_OK)
        {
                yf_log_error(YF_LOG_WARN, log, 0, "lock tq failed");
                goto failed;
        }
        assert(child_no != YF_ANY_CHILD_NO);

        yf_memzero_st(task_info);
        task_info.inqueue_time = yf_now_times.clock_time;
        task_info.id = task_id;
        task_info.timeout_ms = yf_min(1<<20, timeout_ms);

        ret = yf_task_push(tq, &task_info, task, len, log);
        if (ret != YF_OK)
        {
                yf_log_error(YF_LOG_WARN, log, 0, "yf_task_push failed");
                goto failed;
        }

        if (bridge_in->task_signal)
                bridge_in->task_signal(bridge_in, tq, child_no, log);

        if (bridge_in->unlock_tq)
                bridge_in->unlock_tq(bridge_in, tq, child_no, log);

        yf_log_debug2(YF_LOG_DEBUG, log, 0, "task id=%lld send to child_%x success", 
                        task_id, child_no);
        return task_id;

failed:
        yf_free_node_to_pool(&bridge_in->task_cb_info_pool, task_cb, log);
        if (tq)
                bridge_in->unlock_tq(bridge_in, tq, child_no, log);
        return -1;
}


//if in block type, call this will block, else will ret quickly with no effects
void yf_poll_task_res(yf_bridge_t* bridge, yf_log_t* log)
{
        yf_int_t  child_no;
        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)bridge;
        assert(yf_check_be_magic(bridge_in));

        if (bridge_in->wait_task_res == NULL)
                return;
        
        bridge_in->wait_task_res(bridge_in, &child_no, log);
        if (child_no < 0)
                return;

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "poll task res, child_%x ret res", child_no);
        yf_bridge_on_task_res_valiable(bridge_in, child_no, log);
}


/*
* ------child's api------
*/
//child call this after born and init
//if in block type, set evt_driver=NULL
yf_int_t yf_attach_bridge(yf_bridge_t* bridge
                , yf_evt_driver_t* evt_driver, yf_task_handle handler, yf_log_t* log)
{
        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)bridge;
        assert(yf_check_be_magic(bridge_in));

        yf_int_t  child_no = bridge_in->child_no(log);

        bridge_in->task_handler = handler;
        
        bridge_in->task_buf[child_no] = yf_alloc(bridge_in->ctx.max_task_size);
        
        return bridge_in->attach_bridge(bridge_in, child_no, log);
}


//if in block type, call this will block, else will ret quickly with no effects
void yf_poll_task(yf_bridge_t* bridge, yf_log_t* log)
{
        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)bridge;
        assert(yf_check_be_magic(bridge_in));

        if (bridge_in->wait_task == NULL)
                return;

        yf_int_t child_no = bridge_in->child_no(log);
        
        bridge_in->wait_task(bridge_in, child_no, log);
        yf_bridge_on_task_valiable(bridge_in, child_no, log);
}


//if task_res==NULLL or len==0, then no res body
yf_int_t yf_send_task_res_in(yf_bridge_in_t* bridge_in
                , void* task_res, size_t len, task_info_t* task_info, yf_log_t* log)
{
        yf_task_cb_info_t* task_cb;
        yf_int_t  ret;
        yf_task_queue_t* tq = NULL;
        yf_int_t child_no = bridge_in->child_no(log);

        ret = bridge_in->lock_res_tq(bridge_in, &tq, child_no, log);
        if (ret != YF_OK)
        {
                yf_log_error(YF_LOG_WARN, log, 0, "lock res tq failed");
                return YF_ERROR;
        }

        if (len > bridge_in->ctx.max_task_size)
        {
                yf_log_error(YF_LOG_ERR, log, 0, "task res len=%d too big > max_task_size=%d", 
                                len, bridge_in->ctx.max_task_size);
                
                //set error, and len == 0
                task_info->error = 1;
                len = 0;
        }

        ret = yf_task_push(tq, &task_info, task_res, len, log);
        
        if (ret == YF_OK && bridge_in->task_res_signal)
        {
                bridge_in->task_res_signal(bridge_in, tq, child_no, log);
        }

        if (bridge_in->unlock_res_tq)
                bridge_in->unlock_res_tq(bridge_in, tq, child_no, log);

        if (ret != YF_OK)
        {
                yf_log_error(YF_LOG_WARN, log, 0, "task res id=%lld by child_%x send back failed", 
                        task_info->id, child_no);
        }
        else {
                yf_log_debug2(YF_LOG_DEBUG, log, 0, "task res id=%lld by child_%x send back success", 
                        task_info->id, child_no);
        }

        return ret;
}


yf_int_t yf_send_task_res(yf_bridge_t* bridge
                , void* task_res, size_t len, yf_u64_t id
                , yf_int_t status, yf_log_t* log)
{
        task_info_t task_info = {id, status, 0, 0, 0};

        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)bridge;
        assert(yf_check_be_magic(bridge_in));
        
        return  yf_send_task_res_in(bridge_in, task_res, len, &task_info, log);
}



void  yf_bridge_on_task_valiable(yf_bridge_in_t* bridge_in
                , yf_int_t child_no, yf_log_t* log)
{
        task_info_t  task_info;
        yf_int_t  ret;
        yf_s64_t  diff_ms;
        yf_task_queue_t* tq = NULL;
        char* task_buf = bridge_in->task_buf[child_no];
        
        assert(task_buf);
        size_t  task_len;

        while (1)
        {
                task_len = bridge_in->ctx.max_task_size;
                
                ret = bridge_in->lock_tq(bridge_in, &tq, &child_no, log);
                if (ret != YF_OK)
                {
                        yf_log_error(YF_LOG_WARN, log, 0, "lock tq failed");
                        return;
                }

                ret = yf_task_pop(tq, &task_info, task_buf, &task_len, log);

                if (bridge_in->unlock_tq)
                        bridge_in->unlock_tq(bridge_in, tq, child_no, log);

                if (ret == YF_OK)
                {
                        yf_log_debug2(YF_LOG_DEBUG, log, 0, 
                                        "task id=%lld recevied by child_%x success", 
                                        task_info.id, child_no);

                        diff_ms = yf_time_diff_ms(&yf_now_times.clock_time, 
                                        &task_info.inqueue_time);
                        
                        if (task_info.timeout_ms && diff_ms > task_info.timeout_ms)
                        {
                                yf_log_error(YF_LOG_WARN, log, 0, 
                                                "timeout task id=%lld recevied by child_%x, send timeout resp", 
                                                task_info.id, child_no);

                                task_info.timeout = 1;
                                yf_send_task_res_in(bridge_in, NULL, 0, &task_info, log);
                        }
                        else {
                                bridge_in->task_handler((yf_bridge_t*)bridge_in, 
                                                task_buf, task_len, task_info.id, log);
                        }
                        continue;
                }

                if (ret == YF_AGAIN)
                {
                        yf_log_debug0(YF_LOG_DEBUG, log, 0, "no task, wait again...");
                }
                else
                {
                        yf_log_error(YF_LOG_WARN, log, 0, "pop task failed");
                }
                return;
        }
}


void  yf_bridge_on_task_res_valiable(yf_bridge_in_t* bridge_in
                , yf_int_t child_no, yf_log_t* log)
{
        task_info_t  task_info;
        yf_int_t  ret;
        yf_int_t  status;
        yf_task_cb_info_t* task_cb;
        yf_task_queue_t* tq = NULL;
        char* task_buf = bridge_in->task_res_buf;
        
        assert(task_buf);
        size_t  task_len;

        while (1)
        {
                task_len = bridge_in->ctx.max_task_size;
                
                ret = bridge_in->lock_res_tq(bridge_in, &tq, child_no, log);
                if (ret != YF_OK)
                {
                        yf_log_error(YF_LOG_WARN, log, 0, "lock tq failed");
                        return;
                }

                ret = yf_task_pop(tq, &task_info, task_buf, &task_len, log);

                if (bridge_in->unlock_tq)
                        bridge_in->unlock_tq(bridge_in, tq, child_no, log);

                if (ret == YF_OK)
                {
                        task_cb = yf_get_node_by_id(&bridge_in->task_cb_info_pool, 
                                        task_info.id, log);
                        if (task_cb == NULL)
                        {
                                yf_log_error(YF_LOG_WARN, log, 0, 
                                        "task id=%lld sent by child_%x cant found now", 
                                        task_info.id, child_no);
                                continue;
                        }

                        yf_free_node_to_pool(&bridge_in->task_cb_info_pool, task_cb, log);
                        
                        status = task_info.status;
                        if (task_info.timeout)
                                status = YF_TASK_TIMEOUT;
                        else if (task_info.error)
                                status = YF_TASK_ERROR;
                        
                        bridge_in->task_res_handler((yf_bridge_t*)bridge_in, 
                                        task_buf, task_len, task_info.id, 
                                        status, task_cb->data, log);
                        continue;
                }

                if (ret == YF_AGAIN)
                {
                        yf_log_debug0(YF_LOG_DEBUG, log, 0, "no task res, wait again...");
                }
                else
                {
                        yf_log_error(YF_LOG_WARN, log, 0, "pop task res failed");
                }
                return;
        }        
}

