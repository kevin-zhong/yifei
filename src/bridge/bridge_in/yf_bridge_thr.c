#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include "yf_bridge_in.h"


yf_int_t  yf_bridge_et_creator(yf_bridge_in_t* bridge_in, yf_log_t* log);
yf_int_t  yf_bridge_bt_creator(yf_bridge_in_t* bridge_in, yf_log_t* log);

//TODO, performance...
#define yf_bridge_tno(bridge, bridge_bt, child_no) \
        yf_tid_t tid = yf_thread_self(); \
        yf_int_t i1 = 0; \
        for ( i1 = 0; i1 <  bridge->ctx.child_num; i1++ ) \
        { \
                if (bridge_bt->tids[i1] == tid) {\
                        child_no = i1; \
                        break; \
                } \
        } \


/*
* et creator...
*/
typedef struct yf_bridge_thr_s
{
        yf_bridge_channel_t* channels;
        yf_socket_t* socks;
        yf_tid_t* tids;
        yf_task_queue_t** tqs;
        yf_task_queue_t** rtqs;
}
yf_bridge_thr_t;


static yf_int_t yf_bridge_thr_lock_tq(yf_bridge_in_t* bridge
                , yf_task_queue_t** tq, yf_int_t* child_no, yf_log_t* log)
{
        yf_bridge_thr_t* bridge_thr = (yf_bridge_thr_t*)bridge->bridge_data;
        
        if (*child_no == YF_ANY_CHILD_NO)
        {
                *child_no = yf_bridge_get_idle_tq(bridge);
        }
        
        *tq = bridge_thr->tqs[*child_no];
        return YF_OK;
}


static yf_int_t yf_bridge_thr_lock_res_tq(yf_bridge_in_t* bridge
                , yf_task_queue_t** tq, yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_thr_t* bridge_thr = (yf_bridge_thr_t*)bridge->bridge_data;
        
        *tq = bridge_thr->rtqs[child_no];
        return YF_OK;
}


static void yf_bridge_thr_task_signal(yf_bridge_in_t* bridge
                , yf_task_queue_t* tq, yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_thr_t* bridge_thr = (yf_bridge_thr_t*)bridge->bridge_data;

        yf_bridge_channel_signal(bridge_thr->channels + child_no, 1, log);
}

static yf_int_t yf_bridge_thr_attach_res_bridge(yf_bridge_in_t* bridge
                , yf_evt_driver_t* evt_driver, yf_log_t* log)
{
        yf_int_t i1 = 0;
        yf_int_t ret;
        yf_bridge_thr_t* bridge_thr = (yf_bridge_thr_t*)bridge->bridge_data;
        
        for ( i1 = 0; i1 <  bridge->ctx.child_num; i1++ )
        {
                ret = yf_bridge_channel_init(bridge_thr->channels + i1, 
                                bridge, bridge_thr->socks + 2*i1, 
                                evt_driver, 1, i1, log);
                if (ret != YF_OK)
                {
                        yf_log_error(YF_LOG_WARN, log, 0, "init channel_%d failed", i1);
                        return ret;
                }
        }
        return YF_OK;
}


static void yf_bridge_thr_destory(yf_bridge_in_t* bridge, yf_log_t* log)
{
        yf_bridge_thr_t* bridge_thr = (yf_bridge_thr_t*)bridge->bridge_data;

        yf_int_t i1 = 0;
        for ( i1 = 0; i1 <  bridge->ctx.child_num; i1++ )
        {
                //TODO collect threads...
                yf_bridge_channel_uninit(bridge_thr->channels + i1, 1, log);
                yf_close_channel(bridge_thr->socks + 2*i1, log);
        }
        
        yf_free(bridge_thr);
}


static yf_int_t yf_bridge_thr_child_no(yf_bridge_in_t* bridge, yf_log_t* log)
{
        yf_bridge_thr_t* bridge_thr = (yf_bridge_thr_t*)bridge->bridge_data;
        yf_int_t  child_no = -1;
        yf_bridge_tno(bridge, bridge_thr, child_no);
        return child_no;
}


static void yf_bridge_thr_task_res_signal(yf_bridge_in_t* bridge
                , yf_task_queue_t* tq, yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_thr_t* bridge_thr = (yf_bridge_thr_t*)bridge->bridge_data;

        yf_bridge_channel_signal(bridge_thr->channels + child_no, 0, log);
}


static yf_int_t yf_bridge_thr_attach_bridge(yf_bridge_in_t* bridge
                , yf_evt_driver_t* evt_driver, yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_thr_t* bridge_thr = (yf_bridge_thr_t*)bridge->bridge_data;

        yf_int_t ret = yf_bridge_channel_init(bridge_thr->channels + child_no, 
                        bridge, bridge_thr->socks + 2*child_no, 
                        evt_driver, 0, child_no, log);
        if (ret != YF_OK)
        {
                yf_log_error(YF_LOG_WARN, log, 0, "init channel_%d failed", child_no);
        }
        return ret;
}

static void yf_bridge_thr_wait_task(yf_bridge_in_t* bridge
                , yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_thr_t* bridge_thr = (yf_bridge_thr_t*)bridge->bridge_data;
        
        yf_bridge_channel_wait(bridge_thr->channels + child_no, 0, log);
}

yf_int_t  yf_bridge_et_creator(yf_bridge_in_t* bridge_in, yf_log_t* log)
{
        yf_int_t i1 = 0;
        yf_int_t  ret;
        char* addr;
        
        size_t  channls_size = sizeof(yf_bridge_channel_t) * bridge_in->ctx.child_num;
        size_t  socks_size = yf_align_mem(sizeof(yf_socket_t) * bridge_in->ctx.child_num * 2);
        size_t  tid_size = yf_align_mem(sizeof(yf_tid_t) * bridge_in->ctx.child_num);
        size_t  tq_size = sizeof(yf_task_queue_t*) * bridge_in->ctx.child_num;
        size_t  tq_cisze = bridge_in->ctx.child_num 
                                * bridge_in->ctx.queue_capacity;
        
        size_t  all_size = sizeof(yf_bridge_thr_t) 
                        + channls_size + socks_size + tid_size 
                        + 2 * tq_size + 2 * tq_cisze;
        
        yf_bridge_thr_t* bridge_thr = yf_alloc(all_size);
        CHECK_RV(bridge_thr==NULL, YF_ERROR);
        yf_memzero(bridge_thr, all_size);

        bridge_in->bridge_data = bridge_thr;

        bridge_thr->channels = yf_mem_off(bridge_thr, sizeof(yf_bridge_thr_t));
        bridge_thr->socks = yf_mem_off(bridge_thr->channels, channls_size);
        bridge_thr->tids = yf_mem_off(bridge_thr->socks, socks_size);
        bridge_thr->tqs = yf_mem_off(bridge_thr->tids, tid_size);
        bridge_thr->rtqs = yf_mem_off(bridge_thr->tqs, tq_size);
        addr = yf_mem_off(bridge_thr->rtqs, tq_size);

        for ( i1 = 0; i1 < bridge_in->ctx.child_num; i1++ )
        {
                bridge_thr->tqs[i1] = yf_init_task_queue(
                                yf_mem_off(addr, i1 * bridge_in->ctx.queue_capacity), 
                                bridge_in->ctx.queue_capacity, log);
                
                bridge_thr->rtqs[i1] = yf_init_task_queue(
                                yf_mem_off(addr, tq_cisze + i1 * bridge_in->ctx.queue_capacity), 
                                bridge_in->ctx.queue_capacity, log);
                
                if (bridge_thr->tqs[i1] == NULL || bridge_thr->rtqs[i1] == NULL)
                {
                        yf_free(bridge_thr);
                        yf_log_error(YF_LOG_ERR, log, 0, "init tq failed, i1=%d", i1);
                        return  YF_ERROR;
                }
        }

        //set actions
        __yf_bridge_set_ac(bridge_in, lock_tq, yf_bridge_thr);
        __yf_bridge_set_ac(bridge_in, lock_res_tq, yf_bridge_thr);
        __yf_bridge_set_ac(bridge_in, task_signal, yf_bridge_thr); 
        __yf_bridge_set_ac(bridge_in, attach_res_bridge, yf_bridge_thr);
        __yf_bridge_set_ac(bridge_in, destory, yf_bridge_thr);
        __yf_bridge_set_ac(bridge_in, child_no, yf_bridge_thr);
        __yf_bridge_set_ac(bridge_in, task_res_signal, yf_bridge_thr);
        __yf_bridge_set_ac(bridge_in, attach_bridge, yf_bridge_thr);
        __yf_bridge_set_ac(bridge_in, wait_task, yf_bridge_thr);        

        for ( i1 = 0; i1 < bridge_in->ctx.child_num ; i1++ )
        {
                //TODO
                ret = yf_open_channel(bridge_thr->socks + 2*i1, 0, 0, 1, log);
                assert(ret == YF_OK);
                
                ret = yf_create_thread(bridge_thr->tids + i1, 
                                (yf_thread_exe_pt)bridge_in->ctx.exec_func, 
                                bridge_in, log);
                assert(ret == 0);
        }

        return YF_OK;        
}



/*
* bt creator...
*/
typedef struct yf_bridge_bt_s
{
        yf_bridge_channel_t  channels;
        yf_socket_t  socks[2];
        yf_tid_t* tids;
        yf_task_queue_t* tqs;
        yf_task_queue_t* rtqs;

        yf_bridge_mutex_t  mutex;
}
yf_bridge_bt_t;


static yf_int_t yf_bridge_bt_lock_tq(yf_bridge_in_t* bridge
                , yf_task_queue_t** tq, yf_int_t* child_no, yf_log_t* log)
{
        yf_bridge_bt_t* bridge_bt = (yf_bridge_bt_t*)bridge->bridge_data;
        *child_no = 0;

        yf_bridge_mutex_lock(&bridge_bt->mutex, log);
        *tq = bridge_bt->tqs;
        return YF_OK;
}


static yf_int_t yf_bridge_bt_lock_res_tq(yf_bridge_in_t* bridge
                , yf_task_queue_t** tq, yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_bt_t* bridge_bt = (yf_bridge_bt_t*)bridge->bridge_data;
        
        yf_bridge_mutex_lock(&bridge_bt->mutex, log);
        *tq = bridge_bt->rtqs;
        return YF_OK;
}


static void yf_bridge_bt_task_signal(yf_bridge_in_t* bridge
                , yf_task_queue_t* tq, yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_bt_t* bridge_bt = (yf_bridge_bt_t*)bridge->bridge_data;
        
        yf_bridge_mutex_signal(&bridge_bt->mutex, 1, log);
}

static yf_int_t yf_bridge_bt_attach_res_bridge(yf_bridge_in_t* bridge
                , yf_evt_driver_t* evt_driver, yf_log_t* log)
{
        yf_int_t ret;
        yf_bridge_bt_t* bridge_bt = (yf_bridge_bt_t*)bridge->bridge_data;
        
        ret = yf_bridge_channel_init(&bridge_bt->channels, 
                        bridge, bridge_bt->socks, 
                        evt_driver, 1, 0, log);
        if (ret != YF_OK)
        {
                yf_log_error(YF_LOG_WARN, log, 0, "init channel failed");
                return ret;
        }
        return YF_OK;
}


static void yf_bridge_bt_destory(yf_bridge_in_t* bridge, yf_log_t* log)
{
        yf_bridge_bt_t* bridge_bt = (yf_bridge_bt_t*)bridge->bridge_data;
        yf_log_debug(YF_LOG_DEBUG, log, 0, "bridge bt destory...");
        return;

        //TODO collect bteads...
        yf_bridge_channel_uninit(&bridge_bt->channels, 1, log);
        yf_close_channel(bridge_bt->socks, log);
        yf_bridge_mutex_uninit(&bridge_bt->mutex, log);
        
        yf_free(bridge_bt);
}


static yf_int_t yf_bridge_bt_child_no(yf_bridge_in_t* bridge, yf_log_t* log)
{
        yf_bridge_bt_t* bridge_bt = (yf_bridge_bt_t*)bridge->bridge_data;
        yf_int_t  child_no = -1;
        yf_bridge_tno(bridge, bridge_bt, child_no);

        //yf_log_debug1(YF_LOG_DEBUG, log, 0, "child_no=%d", child_no);
        return child_no;
}


static void yf_bridge_bt_task_res_signal(yf_bridge_in_t* bridge
                , yf_task_queue_t* tq, yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_bt_t* bridge_bt = (yf_bridge_bt_t*)bridge->bridge_data;

        yf_bridge_channel_signal(&bridge_bt->channels, 0, log);
}


static yf_int_t yf_bridge_bt_attach_bridge(yf_bridge_in_t* bridge
                , yf_evt_driver_t* evt_driver, yf_int_t child_no, yf_log_t* log)
{
        yf_update_time(NULL, NULL, log);
        return YF_OK;
}

static void yf_bridge_bt_wait_task(yf_bridge_in_t* bridge
                , yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_bt_t* bridge_bt = (yf_bridge_bt_t*)bridge->bridge_data;
        
        yf_bridge_mutex_wait(&bridge_bt->mutex, log);

        //should update time now after waking up...
        yf_update_time(NULL, NULL, log);
}


static void yf_bridge_bt_unlock_tq(yf_bridge_in_t* bridge
                , yf_task_queue_t* tq, yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_bt_t* bridge_bt = (yf_bridge_bt_t*)bridge->bridge_data;
        yf_bridge_mutex_unlock(&bridge_bt->mutex, log);
}

static void yf_bridge_bt_unlock_res_tq(yf_bridge_in_t* bridge
                , yf_task_queue_t* tq, yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_bt_t* bridge_bt = (yf_bridge_bt_t*)bridge->bridge_data;
        yf_bridge_mutex_unlock(&bridge_bt->mutex, log);
}


yf_int_t  yf_bridge_bt_creator(yf_bridge_in_t* bridge_in, yf_log_t* log)
{
        if (bridge_in->ctx.task_dispatch_type != YF_TASK_DISTPATCH_IDLE)
                return yf_bridge_et_creator(bridge_in, log);

        yf_int_t i1 = 0;
        yf_int_t  ret;
        
        size_t  tid_size = yf_align_mem(sizeof(yf_tid_t) * bridge_in->ctx.child_num);
        size_t  tq_cisze = bridge_in->ctx.child_num 
                                * bridge_in->ctx.queue_capacity;
        
        size_t  all_size = sizeof(yf_bridge_bt_t) + tid_size + 2 * tq_cisze;
        
        yf_bridge_bt_t* bridge_bt = yf_alloc(all_size);
        CHECK_RV(bridge_bt==NULL, YF_ERROR);
        yf_memzero(bridge_bt, all_size);

        bridge_in->bridge_data = bridge_bt;
        
        bridge_bt->tids = yf_mem_off(bridge_bt, sizeof(yf_bridge_bt_t));
        
        bridge_bt->tqs = yf_init_task_queue(yf_mem_off(bridge_bt->tids, tid_size), 
                        tq_cisze, log);
        bridge_bt->rtqs = yf_init_task_queue(yf_mem_off(bridge_bt->tqs, tq_cisze), 
                        tq_cisze, log);
        
        if (bridge_bt->tqs == NULL || bridge_bt->rtqs == NULL)
        {
                yf_free(bridge_bt);
                yf_log_error(YF_LOG_ERR, log, 0, "init tq failed, i1=%d", i1);
                return  YF_ERROR;
        }

        ret = yf_open_channel(bridge_bt->socks, 0, 0, 1, log);
        assert(ret == YF_OK);

        ret = yf_bridge_mutex_init(&bridge_bt->mutex, bridge_in, log);
        assert(ret == YF_OK);

        //set actions
        __yf_bridge_set_ac(bridge_in, lock_tq, yf_bridge_bt);
        __yf_bridge_set_ac(bridge_in, lock_res_tq, yf_bridge_bt);
        __yf_bridge_set_ac(bridge_in, task_signal, yf_bridge_bt); 
        __yf_bridge_set_ac(bridge_in, attach_res_bridge, yf_bridge_bt);
        __yf_bridge_set_ac(bridge_in, destory, yf_bridge_bt);
        __yf_bridge_set_ac(bridge_in, child_no, yf_bridge_bt);
        __yf_bridge_set_ac(bridge_in, task_res_signal, yf_bridge_bt);
        __yf_bridge_set_ac(bridge_in, attach_bridge, yf_bridge_bt);
        __yf_bridge_set_ac(bridge_in, wait_task, yf_bridge_bt);
        __yf_bridge_set_ac(bridge_in, unlock_tq, yf_bridge_bt);
        __yf_bridge_set_ac(bridge_in, unlock_res_tq, yf_bridge_bt);

        for ( i1 = 0; i1 < bridge_in->ctx.child_num ; i1++ )
        {
                //TODO
                ret = yf_create_thread(bridge_bt->tids + i1, 
                                (yf_thread_exe_pt)bridge_in->ctx.exec_func, 
                                bridge_in, log);
                assert(ret == 0);
        }     

        return YF_OK;
}


