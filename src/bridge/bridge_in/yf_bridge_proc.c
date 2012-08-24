#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include "yf_bridge_in.h"


yf_int_t  yf_bridge_pp_creator(yf_bridge_in_t* bridge_in, yf_log_t* log);


typedef struct yf_bridge_proc_s
{
        yf_bridge_channel_t* channels;
        yf_pid_t* pids;
        yf_task_queue_t** tqs;
        yf_task_queue_t** rtqs;
        yf_shm_t  shm;
        yf_int_t  child_no;
        yf_log_t* plog;
        yf_evt_driver_t* evt_driver;
}
yf_bridge_proc_t;


static void yf_bridge_proc_exit_cb(struct  yf_process_s* proc);
static void yf_bridge_child_proc_spawn(yf_bridge_in_t* bridge_in
                , yf_int_t child_no, yf_int_t respawn, yf_log_t* log);

static yf_int_t yf_bridge_proc_lock_tq(yf_bridge_in_t* bridge
                , yf_task_queue_t** tq, yf_int_t* child_no, yf_log_t* log)
{
        yf_bridge_proc_t* bridge_proc = (yf_bridge_proc_t*)bridge->bridge_data;
        
        if (*child_no == YF_ANY_CHILD_NO)
        {
                *child_no = yf_bridge_get_idle_tq(bridge);
        }
        
        *tq = bridge_proc->tqs[*child_no];
        return YF_OK;
}


static yf_int_t yf_bridge_proc_lock_res_tq(yf_bridge_in_t* bridge
                , yf_task_queue_t** tq, yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_proc_t* bridge_proc = (yf_bridge_proc_t*)bridge->bridge_data;
        
        *tq = bridge_proc->rtqs[child_no];
        return YF_OK;        
}


static void yf_bridge_proc_task_signal(yf_bridge_in_t* bridge
                , yf_task_queue_t* tq, yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_proc_t* bridge_proc = (yf_bridge_proc_t*)bridge->bridge_data;

        yf_bridge_channel_signal(bridge_proc->channels + child_no, 1, log);
}

static yf_int_t yf_bridge_proc_attach_res_bridge(yf_bridge_in_t* bridge
                , yf_evt_driver_t* evt_driver, yf_log_t* log)
{
        yf_int_t i1 = 0;
        yf_int_t ret;
        yf_process_t* proc;
        yf_bridge_proc_t* bridge_proc = (yf_bridge_proc_t*)bridge->bridge_data;
        bridge_proc->evt_driver = evt_driver;
        
        for ( i1 = 0; i1 <  bridge->ctx.child_num; i1++ )
        {
                proc = yf_processes + yf_pid_slot(bridge_proc->pids[i1]);
                
                ret = yf_bridge_channel_init(bridge_proc->channels + i1, 
                                bridge, proc->channel, 
                                evt_driver, 1, i1, log);
                if (ret != YF_OK)
                {
                        yf_log_error(YF_LOG_WARN, log, 0, "init channel_%d failed", i1);
                        return ret;
                }
        }
        return YF_OK;
}


static void yf_bridge_proc_destory(yf_bridge_in_t* bridge, yf_log_t* log)
{
        yf_int_t i1 = 0;
        yf_process_t* proc;
        yf_bridge_proc_t* bridge_proc = (yf_bridge_proc_t*)bridge->bridge_data;

        for ( i1 = 0; i1 <  bridge->ctx.child_num; i1++ )
        {
                proc = yf_processes + yf_pid_slot(bridge_proc->pids[i1]);
                
                if (proc->pid == YF_INVALID_PID)
                        continue;

                proc->exit_cb = NULL;
                
                if (kill(proc->pid, yf_signal_value(YF_TERMINATE_SIGNAL)) != 0)
                {
                        yf_log_error(YF_LOG_WARN, log, yf_errno, 
                                        "collect child_%d, pid=%P failed", 
                                        i1, proc->pid);
                }
                
                yf_bridge_channel_uninit(bridge_proc->channels + i1, 1, log);
                yf_close_channel(bridge_proc->channels[i1].channels, log);
        }
        
        yf_named_shm_destory(&bridge_proc->shm);
        yf_free(bridge_proc);
}


static yf_int_t yf_bridge_proc_child_no(yf_bridge_in_t* bridge, yf_log_t* log)
{
        yf_bridge_proc_t* bridge_proc = (yf_bridge_proc_t*)bridge->bridge_data;
        return  bridge_proc->child_no;
}


static void yf_bridge_proc_task_res_signal(yf_bridge_in_t* bridge
                , yf_task_queue_t* tq, yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_proc_t* bridge_proc = (yf_bridge_proc_t*)bridge->bridge_data;

        yf_bridge_channel_signal(bridge_proc->channels + child_no, 0, log);
}


static yf_int_t yf_bridge_proc_attach_bridge(yf_bridge_in_t* bridge
                , yf_evt_driver_t* evt_driver, yf_int_t child_no, yf_log_t* log)
{
        yf_process_t* proc = yf_processes + yf_process_slot;
        yf_bridge_proc_t* bridge_proc = (yf_bridge_proc_t*)bridge->bridge_data;

        yf_int_t ret = yf_bridge_channel_init(bridge_proc->channels + child_no, 
                        bridge, proc->channel, 
                        evt_driver, 0, child_no, log);
        if (ret != YF_OK)
        {
                yf_log_error(YF_LOG_WARN, log, 0, "init channel_%d failed", child_no);
        }
        else
                yf_log_debug1(YF_LOG_DEBUG, log, 0, "init channel_%d success", child_no);
        return ret;
}


static void yf_bridge_proc_wait_task(yf_bridge_in_t* bridge
                , yf_int_t child_no, yf_log_t* log)
{
        yf_bridge_proc_t* bridge_proc = (yf_bridge_proc_t*)bridge->bridge_data;
        
        yf_bridge_channel_wait(bridge_proc->channels + child_no, 0, log);        
}


yf_int_t  yf_bridge_pp_creator(yf_bridge_in_t* bridge_in, yf_log_t* log)
{
        yf_int_t i1 = 0;
        yf_int_t  ret;
        yf_task_queue_t* tq;
        
        size_t  channls_size = sizeof(yf_bridge_channel_t) * bridge_in->ctx.child_num;
        size_t  pid_size = yf_align_mem(sizeof(yf_pid_t) * bridge_in->ctx.child_num);
        size_t  tq_size = sizeof(yf_task_queue_t*) * bridge_in->ctx.child_num;
        
        size_t  all_size = sizeof(yf_bridge_proc_t) 
                        + channls_size + pid_size + 2 * tq_size;
        
        yf_bridge_proc_t* bridge_proc = yf_alloc(all_size);
        CHECK_RV(bridge_proc==NULL, YF_ERROR);
        yf_memzero(bridge_proc, all_size);

        bridge_in->bridge_data = bridge_proc;

        bridge_proc->plog = log;

        bridge_proc->channels = yf_mem_off(bridge_proc, sizeof(yf_bridge_proc_t));
        bridge_proc->pids = yf_mem_off(bridge_proc->channels, channls_size);
        bridge_proc->tqs = yf_mem_off(bridge_proc->pids, pid_size);
        bridge_proc->rtqs = yf_mem_off(bridge_proc->tqs, tq_size);

        bridge_proc->shm.key = YF_INVALID_SHM_KEY;
        bridge_proc->shm.log = log;
        bridge_proc->shm.size = 2 * bridge_in->ctx.child_num 
                                * bridge_in->ctx.queue_capacity;
        yf_str_set(&bridge_proc->shm.name, "bridge_proc");

        ret = yf_named_shm_attach(&bridge_proc->shm);
        if (ret != YF_OK)
        {
                yf_free(bridge_proc);
                return YF_ERROR;
        }

        for ( i1 = 0; i1 < bridge_in->ctx.child_num; i1++ )
        {
                bridge_proc->tqs[i1] = yf_init_task_queue(
                                yf_mem_off(bridge_proc->shm.addr, i1 * bridge_in->ctx.queue_capacity), 
                                bridge_in->ctx.queue_capacity, log);
                
                bridge_proc->rtqs[i1] = yf_init_task_queue(
                                yf_mem_off(bridge_proc->shm.addr, 
                                        (bridge_proc->shm.size>>1) + i1 * bridge_in->ctx.queue_capacity), 
                                bridge_in->ctx.queue_capacity, log);
                
                if (bridge_proc->tqs[i1] == NULL || bridge_proc->rtqs[i1] == NULL)
                {
                        yf_named_shm_destory(&bridge_proc->shm);
                        yf_free(bridge_proc);
                        yf_log_error(YF_LOG_ERR, log, 0, "init tq failed, i1=%d", i1);
                        return  YF_ERROR;
                }
        }

        //set actions
        __yf_bridge_set_ac(bridge_in, lock_tq, yf_bridge_proc);
        __yf_bridge_set_ac(bridge_in, lock_res_tq, yf_bridge_proc);
        __yf_bridge_set_ac(bridge_in, task_signal, yf_bridge_proc); 
        __yf_bridge_set_ac(bridge_in, attach_res_bridge, yf_bridge_proc);
        __yf_bridge_set_ac(bridge_in, destory, yf_bridge_proc);
        __yf_bridge_set_ac(bridge_in, child_no, yf_bridge_proc); 
        __yf_bridge_set_ac(bridge_in, task_res_signal, yf_bridge_proc);
        __yf_bridge_set_ac(bridge_in, attach_bridge, yf_bridge_proc);
        __yf_bridge_set_ac(bridge_in, wait_task, yf_bridge_proc);        

        for ( i1 = 0; i1 < bridge_in->ctx.child_num ; i1++ )
        {
                yf_bridge_child_proc_spawn(bridge_in, i1, -1, log);
        }

        return YF_OK;
}


void yf_bridge_proc_exit_cb(struct  yf_process_s* proc)
{
        yf_int_t  child_no, ret;
        yf_bridge_channel_t* bridge_chl;
        yf_bridge_in_t* bridge_in = (yf_bridge_in_t*)proc->data;
        yf_bridge_proc_t* bridge_proc = (yf_bridge_proc_t*)bridge_in->bridge_data;

        for (child_no = 0; child_no < bridge_in->ctx.child_num; ++child_no)
        {
                if (proc->pid == bridge_proc->pids[child_no])
                        break;
        }
        assert(child_no < bridge_in->ctx.child_num);

        bridge_chl = bridge_proc->channels + child_no;
        
        yf_bridge_channel_uninit(bridge_chl, 1, bridge_proc->plog);
        yf_close_channel(bridge_chl->channels, bridge_proc->plog);

        yf_log_error(YF_LOG_WARN, bridge_proc->plog, 0, 
                        "bridge child_%d respawn, exit_err=%d, exit_status=%d", 
                        child_no, 
                        yf_proc_exit_err(proc->status), 
                        yf_proc_exit_code(proc->status));

        yf_bridge_child_proc_spawn(bridge_in, child_no, 
                        yf_pid_slot(proc->pid), bridge_proc->plog);

        ret = yf_bridge_channel_init(bridge_chl, bridge_in, proc->channel, 
                                        bridge_proc->evt_driver, 1, 
                                        child_no, bridge_proc->plog);
        assert(ret == YF_OK);
}


static void yf_bridge_child_proc_spawn(yf_bridge_in_t* bridge_in
                , yf_int_t child_no, yf_int_t respawn, yf_log_t* log)
{
        yf_bridge_proc_t* bridge_proc = (yf_bridge_proc_t*)bridge_in->bridge_data;
        
        bridge_proc->child_no = child_no;
        bridge_proc->pids[child_no] = yf_spawn_process(
                        (yf_spawn_proc_pt)bridge_in->ctx.exec_func, 
                        bridge_in, "bridge", 
                        respawn<0 ? YF_PROC_CHILD : respawn, yf_bridge_proc_exit_cb, log);
        
        assert (bridge_proc->pids[child_no] != YF_INVALID_PID);        
}
