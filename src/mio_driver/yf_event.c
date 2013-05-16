#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
#include "event_in/yf_event_base_in.h"

const yf_str_t yf_evt_type_n[] = {yf_str(""), yf_str("read_evt"), yf_str("write_evt")};

static yf_lock_t yf_evt_lock = YF_LOCK_INITIALIZER;

static yf_int_t  yf_evt_driver_cnt = 0;

yf_evt_driver_t*  yf_evt_driver_create(yf_evt_driver_init_t* driver_init)
{
        char* err_desc[] = {"", "cant create more than one sig evt driver", 
                        "just can create one evt dirver in each proc"};
        yf_int_t  chck_ret = 0;
        
        yf_lock(&yf_evt_lock);
        if (yf_evt_driver_cnt >= 1)
        {
#if !defined (YF_MULTI_EVT_DRIVER)
                chck_ret = 2;
#endif
        }
        yf_unlock(&yf_evt_lock);
        
        if (chck_ret)
        {
                yf_log_error(YF_LOG_ERR, driver_init->log, 0, err_desc[chck_ret]);
                return NULL;
        }
        
        yf_evt_driver_in_t* evt_driver = yf_alloc(sizeof(yf_evt_driver_in_t));
        CHECK_RV(evt_driver == NULL, NULL);

        yf_memzero(evt_driver, sizeof(yf_evt_driver_in_t));

        yf_set_be_magic(evt_driver);
        
        evt_driver->driver_ctx = *driver_init;

        if (yf_init_fd_driver(&evt_driver->fd_driver, driver_init->nfds, 
                        driver_init->poll_type, evt_driver->driver_ctx.log) != YF_OK)
        {
                yf_evt_driver_destory((yf_evt_driver_t*)evt_driver);
                return NULL;
        }
        evt_driver->fd_driver_inited = 1;
        evt_driver->fd_driver.evt_poll->log = evt_driver->driver_ctx.log;
        
        if (yf_init_tm_driver(&evt_driver->tm_driver, driver_init->nstimers, 
                        evt_driver->driver_ctx.log) != YF_OK)
        {
                yf_evt_driver_destory((yf_evt_driver_t*)evt_driver);
                return NULL;
        }
        evt_driver->tm_driver_inited = 1;

        // if in main thread, then init sig driver...
        if (yf_thread_self() == yf_main_thread_id)
        {
                yf_log_debug(YF_LOG_DEBUG, driver_init->log, 0, "main thread, init sig dirver...");
                evt_driver->sig_driver = yf_init_sig_driver(evt_driver->driver_ctx.log);
                evt_driver->sig_driver_inited = 1;
        }

        if (yf_init_proc_driver(&evt_driver->proc_driver, 
                        evt_driver->driver_ctx.log) != YF_OK)
        {
                yf_evt_driver_destory((yf_evt_driver_t*)evt_driver);
                return NULL;
        }
        evt_driver->proc_driver_inited = 1;
        
        evt_driver->tm_driver.log = evt_driver->driver_ctx.log;

        yf_lock(&yf_evt_lock);
        ++yf_evt_driver_cnt;
        yf_unlock(&yf_evt_lock);
        
        return  (yf_evt_driver_t*)evt_driver;
}


void yf_evt_driver_destory(yf_evt_driver_t* driver)
{
        yf_evt_driver_in_t* evt_driver = (yf_evt_driver_in_t*)driver;
        assert(yf_check_be_magic(evt_driver));

        if (evt_driver->driver_ctx.destory_cb)
        {
                evt_driver->driver_ctx.destory_cb(driver, evt_driver->driver_ctx.data, 
                                evt_driver->driver_ctx.log);
        }

        if (evt_driver->fd_driver_inited)
                yf_destory_fd_driver(&evt_driver->fd_driver);
        if (evt_driver->tm_driver_inited)
                yf_destory_tm_driver(&evt_driver->tm_driver);

        if (evt_driver->sig_driver_inited)
        {
                yf_reset_sig_driver(evt_driver->driver_ctx.log);
        }

        if (evt_driver->proc_driver_inited)
        {
                yf_destory_proc_driver(&evt_driver->proc_driver);
                
                yf_lock(&yf_evt_lock);
                --yf_evt_driver_cnt;
                yf_unlock(&yf_evt_lock);
        }
        
        yf_free(evt_driver);
}


yf_evt_driver_init_t* yf_evt_driver_ctx(yf_evt_driver_t* driver)
{
        yf_evt_driver_in_t* evt_driver = (yf_evt_driver_in_t*)driver;
        assert(yf_check_be_magic(evt_driver));
        return &evt_driver->driver_ctx;
}


void yf_evt_driver_stop(yf_evt_driver_t* driver)
{
        yf_evt_driver_in_t* evt_driver = (yf_evt_driver_in_t*)driver;
        assert(yf_check_be_magic(evt_driver));
        
        evt_driver->exit = 1;
}

void yf_evt_driver_start(yf_evt_driver_t* driver)
{
        yf_evt_driver_in_t* evt_driver = (yf_evt_driver_in_t*)driver;
        assert(yf_check_be_magic(evt_driver));
        
        yf_log_t* log = evt_driver->driver_ctx.log;
        evt_driver->exit = 0;

        if (evt_driver->driver_ctx.start_cb)
        {
                evt_driver->driver_ctx.start_cb(driver, evt_driver->driver_ctx.data, 
                                evt_driver->driver_ctx.log);
        }

        yf_u32_t  active_index;

        yf_fd_evt_link_t* iner_evt;
        yf_list_part_t *pos, *keep;
        yf_list_part_t  ready_list;
        yf_init_list_head(&ready_list);
        
        while (!evt_driver->exit)
        {
                if (evt_driver->driver_ctx.poll_cb) 
                {
                        evt_driver->driver_ctx.poll_cb(driver, evt_driver->driver_ctx.data, log);
                }

                //poll fd event;
                evt_driver->fd_driver.evt_poll->poll_cls->actions.dispatch(
                                evt_driver->fd_driver.evt_poll);

                evt_driver->fd_driver.active_op_index++;

                yf_update_time(yf_on_time_reset, &evt_driver->tm_driver, log);
                
                //handle ready list
                yf_list_splice(&evt_driver->fd_driver.ready_list, &ready_list);
                yf_list_for_each_safe(pos, keep, &ready_list)
                {
                        yf_list_del(pos);
                        
                        iner_evt = container_of(pos, yf_fd_evt_link_t, ready_linker);
                        
                        if (iner_evt->timeset)
                        {
                                yf_fd_evt_timer_ctl(&iner_evt->evt, FD_TIMER_DEL, NULL);
                                iner_evt->timeset = 0;
                        }

                        if (!iner_evt->active)//wait for activation
                        {
                                yf_log_debug2(YF_LOG_DEBUG, iner_evt->evt.log, 0, 
                                                "fd=%d not active, evt ready=%V", 
                                                iner_evt->evt.fd, 
                                                &yf_evt_type_n[iner_evt->evt.type]);
                                continue;
                        }

                        active_index = iner_evt->last_active_op_index;
                        iner_evt->evt.fd_evt_handler(&iner_evt->evt);

                        //for oneshot event, so unregister evt after event handled
                        if (iner_evt ->evt.fd > 0 
                                && iner_evt->active && active_index == iner_evt->last_active_op_index)
                        {
                                yf_unregister_fd_evt(&iner_evt->evt);
                        }
                }

                //TODO, if ready list have new evt...
                if (!yf_list_empty(&evt_driver->fd_driver.ready_list))
                {
                        yf_log_debug0(YF_LOG_DEBUG, iner_evt->evt.log, 0, "still have readylist");
                }

                //poll sig event;
                if (evt_driver->sig_driver_inited)
                        evt_driver->sig_driver->poll(evt_driver->sig_driver);

                //poll proc event;
                evt_driver->proc_driver.poll(&evt_driver->proc_driver);
                
                //poll tm event;
                yf_update_time(yf_on_time_reset, &evt_driver->tm_driver, log);

                evt_driver->tm_driver.poll(&evt_driver->tm_driver);
        }

        if (evt_driver->driver_ctx.stop_cb)
        {
                evt_driver->driver_ctx.stop_cb(driver, evt_driver->driver_ctx.data, log);
        }
}


