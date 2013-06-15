#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include "yf_bridge_in.h"

const yf_str_t yf_task_rstatus_n[] = {yf_str("success"), 
                yf_str("timeout"), yf_str("error")};

typedef  yf_int_t  (*yf_bridge_creator_ptr)(yf_bridge_in_t* bridge_in, yf_log_t* log);

#define yf_bridge_creator_index(cx) (cx)->child_ins_type \
                        + ((cx)->parent_ins_type << 1) \
                        + ((cx)->child_poll_type << 2) \
                        + ((cx)->parent_poll_type << 3)

extern  yf_int_t  yf_bridge_pp_creator(yf_bridge_in_t* bridge_in, yf_log_t* log);
extern  yf_int_t  yf_bridge_et_creator(yf_bridge_in_t* bridge_in, yf_log_t* log);
extern  yf_int_t  yf_bridge_bt_creator(yf_bridge_in_t* bridge_in, yf_log_t* log);

static yf_bridge_creator_ptr yf_bridge_ci[16] = 
{
        yf_bridge_pp_creator, 
        yf_bridge_et_creator, //0001(ep->et)
        NULL, 
        yf_bridge_et_creator, //0011(et->et)
        yf_bridge_pp_creator, 
        yf_bridge_bt_creator, //0101(ep->bt)
        NULL, 
        yf_bridge_bt_creator, //0111(et->bt)
        
        NULL, //8
        NULL, NULL, NULL, NULL, NULL, NULL, NULL
};


/*
* ------parent's api------
*/
static void _bridge_task_timeout_handler(yf_tm_evt_t* evt, yf_time_t* start);


//if dispatch_func == NULL, then the idle child will deal the task, else
//the dispated target child will deal the task
yf_bridge_t* yf_bridge_create(yf_bridge_cxt_t* bridge_ctx, yf_log_t* log)
{
        yf_bridge_in_t* bridge_in;
        yf_node_pool_t* node_pool;
        yf_bridge_creator_ptr creator_pfc;
        
        size_t bridge_size = yf_align_mem(sizeof(yf_bridge_in_t));
        size_t task_buf_size = yf_align_mem(bridge_ctx->max_task_size);

        size_t task_cb_size = yf_node_taken_size(sizeof(yf_task_ctx_t));

        CHECK_RV(bridge_ctx->child_num > YF_BRIDGE_MAX_CHILD_NUM 
                        || bridge_ctx->child_num == 0, NULL);
        
        bridge_in = yf_alloc(bridge_size + task_buf_size 
                        + bridge_ctx->max_task_num * task_cb_size);
        
        CHECK_RV(bridge_in == NULL, NULL);
        
        yf_memzero(bridge_in, sizeof(yf_bridge_in_t));
        yf_set_be_magic(bridge_in);
        
        bridge_in->ctx = *bridge_ctx;
        bridge_ctx->queue_capacity = yf_align_mem(bridge_ctx->queue_capacity);
        
        if (bridge_ctx->max_task_size >= (bridge_ctx->queue_capacity>>2))
        {
                yf_log_error(YF_LOG_DEBUG, log, 0, 
                        "max_task_size val=%d not right to queue cap=%d, reset to %d",
                        bridge_ctx->max_task_size, 
                        bridge_ctx->queue_capacity, 
                        bridge_ctx->queue_capacity>>2);
                
                bridge_ctx->max_task_size = bridge_ctx->queue_capacity>>2;
        }
        
        bridge_in->task_res_buf = (char*)bridge_in + bridge_size;

        node_pool = &bridge_in->task_cb_info_pool;

        node_pool->each_taken_size = task_cb_size;
        node_pool->total_num = bridge_ctx->max_task_num;
        node_pool->nodes_array = bridge_in->task_res_buf + task_buf_size;

        yf_init_node_pool(node_pool, log);

        creator_pfc = yf_bridge_ci[yf_bridge_creator_index(bridge_ctx)];
        
        if (creator_pfc == NULL)
        {
                yf_log_error(YF_LOG_WARN, log, 0, "not supported bridge type");
                yf_free(bridge_in);
                return NULL;
        }
        if (creator_pfc(bridge_in, log) != YF_OK)
        {
                yf_log_error(YF_LOG_WARN, log, 0, "bridge init failed");
                yf_free(bridge_in);
                return NULL;
        }
        
        return  (yf_bridge_t*)bridge_in;
}


//will destory childs and destory bridge...
yf_int_t  yf_bridge_destory(yf_bridge_t* bridge, yf_log_t* log)
{
        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)bridge;
        assert(yf_check_be_magic(bridge_in));

        bridge_in->destory(bridge_in, log);

        //TODO, fix bug here...
        if (bridge_in->ctx.child_ins_type == YF_BRIDGE_INS_PROC)
                yf_free(bridge_in);
        return YF_OK;
}


yf_bridge_cxt_t* yf_bridge_ctx(yf_bridge_t* bridge)
{
        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)bridge;
        assert(yf_check_be_magic(bridge_in));
        return  &bridge_in->ctx;
}


yf_int_t yf_attach_res_bridge(yf_bridge_t* bridge
                , yf_evt_driver_t* evt_driver, yf_task_res_handle handler, yf_log_t* log)
{
        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)bridge;
        assert(yf_check_be_magic(bridge_in));

        bridge_in->task_res_handler = handler;
        bridge_in->parent_evt_driver = evt_driver;
        
        return bridge_in->attach_res_bridge(bridge_in, evt_driver, log);
}


//will return taskid>0 if success, else ret -1
yf_u64_t yf_send_task(yf_bridge_t* bridge
                , void* task, size_t len, yf_u32_t hash
                , void* data, yf_u32_t timeout_ms, yf_log_t* log)
{
        yf_task_ctx_t* task_ctx;
        task_info_t  task_info;
        yf_u64_t  task_id;
        yf_int_t  ret;
        yf_int_t  child_no;
        yf_task_queue_t* tq = NULL;
        
        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)bridge;
        assert(yf_check_be_magic(bridge_in));

        if (unlikely(len > bridge_in->ctx.max_task_size))
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

        task_ctx = yf_alloc_node_from_pool(&bridge_in->task_cb_info_pool, log);
        if (unlikely(task_ctx == NULL))
        {
                yf_log_error(YF_LOG_WARN, log, 0, "alloc task cb failed");
                return -1;
        }

        yf_memzero(task_ctx, sizeof(yf_task_ctx_t));
        task_ctx->data = data;
        task_ctx->bridge_in = bridge_in;
        
        task_id = yf_get_id_by_node(&bridge_in->task_cb_info_pool, task_ctx, log);
        
        ret = bridge_in->lock_tq(bridge_in, &tq, &child_no, log);
        if (unlikely(ret != YF_OK))
        {
                yf_log_error(YF_LOG_WARN, log, 0, "lock tq failed");
                goto failed;
        }
        assert(child_no >= 0 && child_no < bridge_in->ctx.child_num);
        task_ctx->child_no = child_no;

        if (timeout_ms == 0 && bridge_in->ctx.child_ins_type == YF_BRIDGE_INS_PROC)
        {
                yf_log_debug(YF_LOG_DEBUG, log, 0, 
                                "need send task to child proc with never tiemout");
                timeout_ms = 7200 * 1000;//two hour
        }

        //set tm evt if can
        if (timeout_ms && bridge_in->parent_evt_driver)
        {
                ret = yf_alloc_tm_evt(bridge_in->parent_evt_driver, &task_ctx->tm_evt, log);
                if (unlikely(ret != YF_OK))
                {
                        yf_log_error(YF_LOG_WARN, log, 0, "alloc tm evt failed");
                        goto failed;
                }

                yf_time_t  tm_set;
                yf_ms_2_time(timeout_ms + 1000, &tm_set);//this timeout just protect task leak...
                task_ctx->tm_evt->data = task_ctx;
                task_ctx->tm_evt->timeout_handler = _bridge_task_timeout_handler;
                
                ret = yf_register_tm_evt(task_ctx->tm_evt, &tm_set);
                assert(ret == YF_OK);
        }

        yf_memzero_st(task_info);
        
        //note, must use wall_time, cause cross proc maybe
        yf_real_walltime(&task_info.inqueue_time);
        task_info.id = task_id;
        task_info.timeout_ms = yf_min(1<<20, timeout_ms);

        ret = yf_task_push(tq, &task_info, task, len, log);
        if (unlikely(ret != YF_OK))
        {
                yf_log_error(YF_LOG_WARN, log, 0, "yf_task_push failed");
                goto failed;
        }

        if (bridge_in->task_signal)
                bridge_in->task_signal(bridge_in, tq, child_no, log);

        if (bridge_in->unlock_tq)
                bridge_in->unlock_tq(bridge_in, tq, child_no, log);

        bridge_in->task_execut[child_no] += 1;

        yf_log_debug5(YF_LOG_DEBUG, log, 0, 
                        "task id=%L send to child_%d success, time=[%d-%d], executing=%d", 
                        task_id, child_no, 
                        task_info.inqueue_time.tv_sec, task_info.inqueue_time.tv_msec,
                        bridge_in->task_execut[child_no]);
        return task_id;

failed:
        if (task_ctx->tm_evt)
        {
                yf_free_tm_evt(task_ctx->tm_evt);
        }
        yf_free_node_to_pool(&bridge_in->task_cb_info_pool, task_ctx, log);
        
        if (tq && bridge_in->unlock_tq)
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

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "poll task res, child_%d ret res", child_no);
        yf_bridge_on_task_res_valiable(bridge_in, child_no, log);
}


static void _bridge_task_timeout_handler(yf_tm_evt_t* evt, yf_time_t* start)
{
        yf_task_ctx_t*  task_ctx = evt->data;
        yf_bridge_in_t* bridge_in = task_ctx->bridge_in;
        assert(task_ctx->tm_evt == evt);

        yf_u64_t task_id = yf_get_id_by_node(&bridge_in->task_cb_info_pool, 
                        task_ctx, evt->log);

        // 1
        bridge_in->task_execut[task_ctx->child_no] -= 1;

        yf_log_error(YF_LOG_WARN, evt->log, 0, 
                        "timeout task id=%L by child_%d timeout arrive, execute=%d", 
                        task_id, task_ctx->child_no, 
                        bridge_in->task_execut[task_ctx->child_no]);
        // 2
        bridge_in->task_res_handler((yf_bridge_t*)bridge_in, 
                        NULL, 0, task_id, 
                        YF_TASK_TIMEOUT, task_ctx->data, evt->log);
        // 3
        yf_free_node_to_pool(&bridge_in->task_cb_info_pool, 
                        task_ctx, evt->log);
        // 4
        yf_free_tm_evt(evt);
}


/*
* ------child's api------
*/

yf_int_t  yf_bridge_child_no(yf_bridge_t* bridge, yf_log_t* log)
{
        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)bridge;
        assert(yf_check_be_magic(bridge_in));
        return  bridge_in->child_no(bridge_in, log);
}

//child call this after born and init
//if in block type, set evt_driver=NULL
yf_int_t yf_attach_bridge(yf_bridge_t* bridge
                , yf_evt_driver_t* evt_driver, yf_task_handle handler, yf_log_t* log)
{
        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)bridge;
        assert(yf_check_be_magic(bridge_in));

        yf_int_t  child_no = bridge_in->child_no(bridge_in, log);
        if (child_no < 0)
        {
                if (bridge_in->attach_child_ins == NULL)
                {
                        yf_log_error(YF_LOG_WARN, log, 0, "unchild ins, no attach pf, attach fail");
                        return YF_ERROR;
                }
                
                child_no = bridge_in->attach_child_ins(bridge_in, log);
                if (child_no < 0)
                {
                        yf_log_error(YF_LOG_WARN, log, 0, "attach child ins failed, maybe too many...");
                        return YF_ERROR;
                }
        }

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "attach bridge index=%d", child_no);

        bridge_in->task_handler = handler;

        if (bridge_in->ctx.child_ins_type == YF_BRIDGE_INS_PROC)
                bridge_in->task_buf[child_no] = bridge_in->task_res_buf;
        else //TODO, free...
                bridge_in->task_buf[child_no] = yf_alloc(bridge_in->ctx.max_task_size);
        
        if (bridge_in->task_buf[child_no] == NULL)
                return YF_ERROR;
        
        return bridge_in->attach_bridge(bridge_in, evt_driver, child_no, log);
}


//if in block type, call this will block, else will ret quickly with no effects
void yf_poll_task(yf_bridge_t* bridge, yf_log_t* log)
{
        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)bridge;
        assert(yf_check_be_magic(bridge_in));

        if (bridge_in->wait_task == NULL)
                return;

        yf_int_t child_no = bridge_in->child_no(bridge_in, log);

        //add on 2013/05/21, if threads called later, and task arrive first, then ...
        yf_bridge_on_task_valiable(bridge_in, child_no, log);
        
        bridge_in->wait_task(bridge_in, child_no, log);
        yf_bridge_on_task_valiable(bridge_in, child_no, log);
}


//if task_res==NULLL or len==0, then no res body
yf_int_t yf_send_task_res_in(yf_bridge_in_t* bridge_in
                , void* task_res, size_t len, task_info_t* task_info, yf_log_t* log)
{
        yf_task_ctx_t* task_ctx;
        yf_int_t  ret;
        yf_task_queue_t* tq = NULL;
        yf_int_t child_no = bridge_in->child_no(bridge_in, log);

        ret = bridge_in->lock_res_tq(bridge_in, &tq, &child_no, log);
        if (unlikely(ret != YF_OK))
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

        ret = yf_task_push(tq, task_info, task_res, len, log);
        
        if (ret == YF_OK && bridge_in->task_res_signal)
        {
                bridge_in->task_res_signal(bridge_in, tq, child_no, log);
        }

        if (bridge_in->unlock_res_tq)
                bridge_in->unlock_res_tq(bridge_in, tq, child_no, log);

        if (unlikely(ret != YF_OK))
        {
                yf_log_error(YF_LOG_WARN, log, 0, "task res id=%L by child_%d send back failed", 
                        task_info->id, child_no);
        }
        else {
                yf_log_debug2(YF_LOG_DEBUG, log, 0, "task res id=%L by child_%d send back success", 
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
        yf_time_t walltime;
        yf_task_queue_t* tq = NULL;
        char* task_buf = bridge_in->task_buf[child_no];
        
        assert(task_buf);
        size_t  task_len;

        while (1)
        {
                task_len = bridge_in->ctx.max_task_size;
                
                ret = bridge_in->lock_tq(bridge_in, &tq, &child_no, log);
                if (unlikely(ret != YF_OK))
                {
                        yf_log_error(YF_LOG_WARN, log, 0, "lock tq failed");
                        return;
                }

                ret = yf_task_pop(tq, &task_info, task_buf, &task_len, log);

                if (bridge_in->unlock_tq)
                        bridge_in->unlock_tq(bridge_in, tq, child_no, log);

                if (likely(ret == YF_OK))
                {
                        yf_real_walltime(&walltime);
                        
                        diff_ms = yf_time_diff_ms(&walltime, &task_info.inqueue_time);
                        
                        yf_log_debug3(YF_LOG_DEBUG, log, 0, 
                                        "task id=%L recevied by child_%d success, diff_ms=%L", 
                                        task_info.id, child_no, diff_ms);
                        
                        if (unlikely(task_info.timeout_ms && diff_ms > task_info.timeout_ms))
                        {
                                yf_log_error(YF_LOG_WARN, log, 0, 
                                                "timeout task id=%L recevied by child_%d, send timeout resp", 
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
        yf_task_ctx_t* task_ctx;
        yf_task_queue_t* tq = NULL;
        char* task_buf = bridge_in->task_res_buf;
        
        assert(task_buf);
        size_t  task_len;

        while (1)
        {
                task_len = bridge_in->ctx.max_task_size;
                
                ret = bridge_in->lock_res_tq(bridge_in, &tq, &child_no, log);
                if (unlikely(ret != YF_OK))
                {
                        yf_log_error(YF_LOG_WARN, log, 0, "lock tq failed");
                        return;
                }

                ret = yf_task_pop(tq, &task_info, task_buf, &task_len, log);

                if (bridge_in->unlock_res_tq)
                        bridge_in->unlock_res_tq(bridge_in, tq, child_no, log);

                if (likely(ret == YF_OK))
                {
                        task_ctx = yf_get_node_by_id(&bridge_in->task_cb_info_pool, 
                                        task_info.id, log);
                        if (unlikely(task_ctx == NULL))
                        {
                                yf_log_error(YF_LOG_WARN, log, 0, 
                                        "task res id=%L sent by child_%d cant found now", 
                                        task_info.id, child_no);
                                continue;
                        }
                        
                        status = task_info.status;
                        if (task_info.timeout)
                                status = YF_TASK_TIMEOUT;
                        else if (task_info.error)
                                status = YF_TASK_ERROR;

                        assert(child_no == task_ctx->child_no);
                        bridge_in->task_execut[child_no] -= 1;

                        if (task_ctx->tm_evt)
                        {
                                yf_free_tm_evt(task_ctx->tm_evt);
                        }
                        
                        yf_log_debug3(YF_LOG_DEBUG, log, 0, 
                                        "task res id=%L sent by childd_%d ret, execute=%d", 
                                        task_info.id, child_no, 
                                        bridge_in->task_execut[child_no]);
                        
                        bridge_in->task_res_handler((yf_bridge_t*)bridge_in, 
                                        task_buf, task_len, task_info.id, 
                                        status, task_ctx->data, log);

                        yf_free_node_to_pool(&bridge_in->task_cb_info_pool, task_ctx, log);
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


yf_int_t  yf_bridge_get_idle_tq(yf_bridge_in_t* bridge_in)
{
        yf_int_t i1 = 0;
        yf_int_t tq_no = 0;
        yf_uint_t tq_cnt = 0xfffffffe;

        for ( i1 = 0; i1 < bridge_in->ctx.child_num; i1++ )
        {
                if (bridge_in->task_execut[i1] < tq_cnt)
                {
                        tq_cnt = bridge_in->task_execut[i1];
                        tq_no = i1;
                }
        }
        return  tq_no;
}

