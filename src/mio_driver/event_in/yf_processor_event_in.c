#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include "yf_event_base_in.h"
#include <mio_driver/yf_send_recv.h>

static yf_int_t  yf_unregist_process_evt_in(yf_processor_event_in_t *pos);
static void  yf_proc_evt_poll(yf_proc_evt_driver_in_t* proc_driver);

static void yf_on_child_channle_rwable(yf_fd_event_t* evt);
static void yf_on_child_exe_timeout(yf_tm_evt_t* evt, yf_time_t* start);
static void yf_on_child_exit(yf_process_t* proc);


yf_int_t   yf_init_proc_driver(yf_proc_evt_driver_in_t* proc_driver
                , yf_log_t* log)
{
        proc_driver->log = log;
        proc_driver->poll = yf_proc_evt_poll;
        
        yf_init_list_head(&proc_driver->proc_alloc_list);
        yf_init_list_head(&proc_driver->proc_evt_list);

        return YF_OK;
}

yf_int_t   yf_destory_proc_driver(yf_proc_evt_driver_in_t* proc_driver)
{
        yf_processor_event_in_t *pos;
        yf_list_part_t * list_part;

        yf_list_for_each_entry(pos, &proc_driver->proc_evt_list, evt_linker)
        {
                yf_unregist_process_evt_in(pos);
        }

        while (!yf_list_empty(&proc_driver->proc_alloc_list))
        {
                list_part = proc_driver->proc_alloc_list.next;
                yf_list_del(list_part);
                
                pos = yf_list_entry(list_part, yf_processor_event_in_t, alloc_linker);
                yf_free(pos);
        }
        return  YF_OK;
}

yf_int_t  yf_alloc_proc_evt(yf_evt_driver_t* driver, yf_processor_event_t** proc_evt
                , yf_log_t* log)
{
        yf_evt_driver_in_t* evt_driver = (yf_evt_driver_in_t*)driver;
        assert(yf_check_be_magic(evt_driver));
        
        yf_proc_evt_driver_in_t* proc_driver = &evt_driver->proc_driver;
        
        yf_processor_event_in_t * evt_tmp;
        evt_tmp = yf_alloc(sizeof(yf_processor_event_in_t));
        yf_memzero(evt_tmp, sizeof(yf_processor_event_in_t));

        evt_tmp->proc_slot = -1;
        evt_tmp->evt.driver = driver;
        evt_tmp->evt.log = log;

        yf_list_add_tail(&evt_tmp->alloc_linker, &proc_driver->proc_alloc_list);

        *proc_evt = &evt_tmp->evt;

        return  YF_OK;
}

yf_int_t  yf_free_proc_evt(yf_processor_event_t* proc_evt)
{
        yf_processor_event_in_t * evt_tmp = container_of(proc_evt, 
                        yf_processor_event_in_t, evt);

        if (evt_tmp->proc_slot >= 0)
        {
                yf_unregist_process_evt_in(evt_tmp);
        }
        assert(yf_list_linked(&evt_tmp->alloc_linker));

        yf_list_del(&evt_tmp->alloc_linker);

        yf_free(evt_tmp);
        return  YF_OK;
}


yf_int_t  yf_unregist_process_evt_in(yf_processor_event_in_t *pos)
{
        yf_pid_t  pid;
        yf_process_t* proc;

        //unregist tm event
        if (pos->tm_evt)
        {
                if (!pos->evt.timeout)
                        yf_unregister_tm_evt(pos->tm_evt);
                
                yf_free_tm_evt(pos->tm_evt);
                pos->tm_evt = NULL;
        }

        //unregist channle event
        if (pos->channel_read_evt || pos->channel_write_evt)
        {
                yf_log_debug1(YF_LOG_DEBUG, pos->evt.log, 0, 
                                "destory rw channel, fd=%d", 
                                pos->channel_read_evt->fd);
                
                yf_unregister_fd_evt(pos->channel_read_evt);
                yf_unregister_fd_evt(pos->channel_write_evt);
                
                yf_free_fd_evt(pos->channel_read_evt, pos->channel_write_evt);
                
                pos->channel_read_evt = NULL;
                pos->channel_write_evt = NULL;
        }        
        
        if (pos->proc_slot == -1)
        {
                yf_log_error(YF_LOG_WARN, pos->evt.log, 0, 
                                "yf_proc no proc (%p) ", pos);
                return  YF_ERROR;
        }

        proc = &yf_processes[pos->proc_slot];
        pid = proc->pid;
        
        proc->exit_cb = NULL;

        //kill proc
        if (!proc->exited)
        {
                if (kill(pid, yf_signal_value(YF_KILL_SIGNAL)) == 0)
                {
                        yf_log_debug1(YF_LOG_DEBUG, pos->evt.log, 0, 
                                     "destory detach process, kill (%P, 9) success", pid);
                }
                else {
                        yf_log_error(YF_LOG_WARN, pos->evt.log, yf_errno, 
                                        "kill (%P, 9) failed", pid);
                }
        }

        //close channel
        if (proc->type != YF_PROC_DETACH)
        {
                yf_log_debug2(YF_LOG_DEBUG, pos->evt.log, 0, "close rw channel [%d-%d]", 
                                proc->channel[0], 
                                proc->channel[1]);
                yf_close_channel(proc->channel, pos->evt.log);
        }

        pos->proc_slot = -1;
        return  YF_OK;
}


void  yf_proc_evt_poll(yf_proc_evt_driver_in_t* proc_driver)
{
}


yf_int_t   yf_register_proc_evt(yf_processor_event_t* proc_evt, yf_time_t  *time_out)
{
        yf_evt_driver_in_t* evt_driver = (yf_evt_driver_in_t*)proc_evt->driver;
        
        yf_processor_event_in_t * proc_evt_inner = container_of(proc_evt, 
                        yf_processor_event_in_t, evt);

        //excutate cmd
        yf_pid_t  new_pid = yf_execute(&proc_evt->exec_ctx, proc_evt->log);
        yf_tm_evt_t* tm_evt;

        if (YF_INVALID_PID == new_pid)
        {
                yf_log_error(YF_LOG_WARN, proc_evt->log, 0, "execute failed, register proc evt..");
                return YF_ERROR;
        }

        proc_evt_inner->proc_slot = yf_pid_slot(new_pid);
        assert(proc_evt_inner->proc_slot >= 0);

        yf_process_t* proc = &yf_processes[proc_evt_inner->proc_slot];

        //send cmd data and recv
        if (proc_evt->exec_ctx.type != YF_PROC_DETACH)
        {
                if (YF_OK != yf_alloc_fd_evt((yf_evt_driver_t*)evt_driver, 
                                proc->channel[0], 
                                &proc_evt_inner->channel_read_evt, 
                                &proc_evt_inner->channel_write_evt, 
                                proc_evt->log))
                                goto fail_end;

                proc_evt_inner->channel_read_evt->fd_evt_handler 
                                = yf_on_child_channle_rwable;
                proc_evt_inner->channel_read_evt->data = proc_evt_inner;
                
                proc_evt_inner->channel_write_evt->fd_evt_handler 
                                = yf_on_child_channle_rwable;
                proc_evt_inner->channel_read_evt->data = proc_evt_inner;
                
                if (yf_proc_writable(proc_evt->exec_ctx.type)
                        && proc_evt->write_chain)
                {
                        proc_evt_inner->channel_write_evt->ready = 1;
                        
                        yf_on_child_channle_rwable(proc_evt_inner->channel_write_evt);
                }
                
                if (yf_proc_readable(proc_evt->exec_ctx.type)
                        && proc_evt->pool)
                {
                        if (yf_register_fd_evt(proc_evt_inner->channel_read_evt, NULL)
                                        != YF_OK)
                                        goto fail_end;
                }
        }

        //set timer
        if (time_out)
        {
                if (YF_OK != yf_alloc_tm_evt((yf_evt_driver_t*)evt_driver, &tm_evt, proc_evt->log))
                        goto fail_end;

                tm_evt->data = proc_evt_inner;
                tm_evt->timeout_handler = yf_on_child_exe_timeout;
                
                if (YF_OK != yf_register_tm_evt(tm_evt, time_out))
                        goto fail_end;

                proc_evt_inner->tm_evt = tm_evt;
        }

        //set ret callback
        proc->exit_cb = yf_on_child_exit;

        proc_evt->processing = 1;

        return  YF_OK;
        
fail_end:
        yf_unregist_process_evt_in(proc_evt_inner);
        return YF_ERROR;
}


yf_int_t   yf_unregister_proc_evt(yf_processor_event_t* proc_evt)
{
        yf_evt_driver_in_t* evt_driver = (yf_evt_driver_in_t*)proc_evt->driver;
        
        yf_processor_event_in_t * proc_evt_inner = container_of(proc_evt, 
                        yf_processor_event_in_t, evt);

        return yf_unregist_process_evt_in(proc_evt_inner);
}


yf_process_t* yf_get_proc_by_evt(yf_processor_event_t* proc_evt)
{
        yf_processor_event_in_t * proc_evt_inner = container_of(proc_evt, 
                        yf_processor_event_in_t, evt);

        if (proc_evt_inner->proc_slot < 0)
                return NULL;
        else
                return  yf_processes + proc_evt_inner->proc_slot;
}


void yf_on_child_channle_rwable(yf_fd_event_t* evt)
{
        yf_processor_event_in_t* proc_evt_inner = (yf_processor_event_in_t*)evt->data;
        yf_processor_event_t* proc_evt = &proc_evt_inner->evt;
        yf_chain_t* chain = NULL;
        yf_chain_t** last_cl_pos = NULL;
        fd_rw_ctx_t rw_ctx;
        yf_bufs_t  bufs;
        yf_int_t  ret;

        assert(evt->ready);

        rw_ctx.fd_evt = evt;
        rw_ctx.pool = proc_evt->pool;
        
        if (evt->type == YF_REVT)
        {
                bufs.num = 1;
                bufs.size = yf_pagesize * 2;
                
                for (last_cl_pos = &proc_evt->read_chain; *last_cl_pos; 
                                last_cl_pos = &(*last_cl_pos)->next)
                {
                        chain = *last_cl_pos;
                }
                
                while (evt->ready)
                {
                        if (chain == NULL || chain->buf->end == chain->buf->last)
                        {
                                chain = yf_create_chain_of_bufs(proc_evt->pool, &bufs);
                                if (chain == NULL)
                                        goto fail_end;

                                *last_cl_pos = chain;
                                last_cl_pos = &chain->next;
                        }

                        rw_ctx.rw_cnt = 0;
                        ret = yf_readv_chain(&rw_ctx, chain);
                        if (ret < 0 && ret != YF_AGAIN)
                                goto fail_end;

                        yf_log_debug1(YF_LOG_DEBUG, proc_evt->log, 0, 
                                        "read %d from chnl", rw_ctx.rw_cnt);
                }

                if (!evt->eof)
                        yf_register_fd_evt(evt, NULL);
        }
        
        else if (evt->type == YF_WEVT)
        {
                rw_ctx.rw_cnt = 0;
                
                chain = yf_writev_chain(&rw_ctx, proc_evt->write_chain, 0);
                
                if (chain == YF_CHAIN_ERROR)
                {
                        proc_evt->error = 1;
                        yf_log_error(YF_LOG_ERR, proc_evt->log, 0, "write to channel failed");
                        goto  fail_end;
                }

                proc_evt->write_chain = chain;
                
                if (chain && yf_buf_size(chain->buf) > 0)
                {
                        yf_log_debug1(YF_LOG_DEBUG, proc_evt->log, 0, 
                                        "rest %d to write to chnl", yf_buf_size(chain->buf));
                        
                        yf_register_fd_evt(evt, NULL);
                }
        }
        else
                assert(0);
        return;

fail_end:
        yf_unregist_process_evt_in(proc_evt_inner);
        proc_evt->ret_handler(proc_evt);
        
}


void yf_on_child_exe_timeout(yf_tm_evt_t* evt, yf_time_t* start)
{
        yf_processor_event_in_t* proc_evt_inner = (yf_processor_event_in_t*)evt->data;
        yf_processor_event_t* proc_evt = &proc_evt_inner->evt;
        yf_process_t* proc = &yf_processes[proc_evt_inner->proc_slot];

        yf_log_error(YF_LOG_WARN, proc_evt->log, 0, "%s(path=%s)  execute timeout", 
                        proc_evt->exec_ctx.name, 
                        proc_evt->exec_ctx.path);

        proc_evt->timeout = 1;
        
        yf_unregist_process_evt_in(proc_evt_inner);
        proc_evt->ret_handler(proc_evt);
}


void yf_on_child_exit(yf_process_t* proc)
{
        //so bt...
        yf_exec_ctx_t* exe_ctx = (yf_exec_ctx_t*)proc->data;
        
        yf_processor_event_t* proc_evt = container_of(exe_ctx, 
                        yf_processor_event_t, exec_ctx);
        yf_processor_event_in_t* proc_evt_inner = container_of(proc_evt, 
                        yf_processor_event_in_t, evt);

        proc_evt->processing = 0;
        proc_evt->error = yf_proc_exit_err(proc->status);
        proc_evt->exit_code = yf_proc_exit_code(proc->status);

        yf_log_debug4(YF_LOG_DEBUG, proc_evt->log, 0, 
                        "%s(path=%s) execute error=%d, exit_code=%d", 
                        proc_evt->exec_ctx.name, 
                        proc_evt->exec_ctx.path, 
                        proc_evt->error, 
                        proc_evt->exit_code);

        yf_unregist_process_evt_in(proc_evt_inner);
        proc_evt->ret_handler(proc_evt);

        yf_log_debug2(YF_LOG_DEBUG, proc_evt->log, 0, "%s(path=%s) ret handle done!", 
                        proc_evt->exec_ctx.name, 
                        proc_evt->exec_ctx.path);
}

