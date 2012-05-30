#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include "yf_event_base_in.h"

yf_int_t   yf_init_fd_driver(yf_fd_evt_driver_in_t* fd_evt_driver
                , yf_u32_t nfds, yf_s32_t  poll_type, yf_log_t* log)
{
        yf_s32_t  try_poll_type;
        
        YF_INIT_LIST_HEAD(&fd_evt_driver->all_evt_list);
        YF_INIT_LIST_HEAD(&fd_evt_driver->ready_list);

        fd_evt_driver->evts_capcity = nfds;
        fd_evt_driver->pevents = yf_alloc(sizeof(yf_fd_evt_in_t) * nfds);
        if (fd_evt_driver->pevents == NULL) {
                yf_log_error(YF_LOG_ERR, log, yf_errno, "alloc pevents failed");
                return YF_ERROR;
        }

        if (poll_type < 0 || poll_type >= yf_fd_poller_cls_num) {
                yf_log_error(YF_LOG_ERR, log, 0, "poll type=%d illegal", poll_type);
                goto init_failed;
        }

        if (poll_type == YF_POLL_BY_DETECT)
                try_poll_type = yf_fd_poller_cls_num - 1;
        else
                try_poll_type = poll_type;

        yf_load_fd_pollers();

try_other_poll:
        if (yf_fd_poller_cls[try_poll_type].name == NULL) {
                yf_log_error(YF_LOG_ERR, log, 0, "poll type=%d not support", try_poll_type);
                goto init_failed;
        }

        fd_evt_driver->evt_poll = yf_fd_poller_cls[try_poll_type].actions.init(nfds);
        if (fd_evt_driver->evt_poll == NULL)
        {
                yf_log_error(YF_LOG_ERR, log, 0, "poll=%d action inited failed", try_poll_type);
                goto init_failed;
        }

        fd_evt_driver->evt_poll->ctx = fd_evt_driver;
        fd_evt_driver->evt_poll->poll_cls = yf_fd_poller_cls + try_poll_type;
        yf_log_debug1(YF_LOG_DEBUG, log, 0, "poll init sucess, type=%s", 
                        yf_fd_poller_cls[try_poll_type].name);
        return  YF_OK;

init_failed:
        if (try_poll_type > 0 && poll_type == YF_POLL_BY_DETECT)
        {
                try_poll_type--;
                goto try_other_poll;
        }
        yf_free((fd_evt_driver)->pevents);
        return  YF_ERROR;        
}


void  yf_destory_fd_driver(yf_fd_evt_driver_in_t *fd_driver) 
{
        if (fd_driver->evt_poll == NULL)
                return;
        
        if (fd_driver->evt_poll->poll_cls->actions.uninit)
                fd_driver->evt_poll->poll_cls->actions.uninit(fd_driver->evt_poll);
        
        yf_free((fd_driver)->pevents);
}


yf_int_t  yf_alloc_fd_evt(yf_evt_driver_t*  driver, yf_fd_t fd
                , yf_fd_event_t** read, yf_fd_event_t** write, yf_log_t* log)
{
        yf_fd_evt_driver_in_t* fd_evt_driver = &((yf_evt_driver_in_t*)driver)->fd_driver;
        if (log == NULL)
                log = fd_evt_driver->evt_poll->log;

        if (fd >= fd_evt_driver->evts_capcity)
        {
                yf_log_error(YF_LOG_ERR, log, 0, "too many open fds, fd=%d, evts_capcity=%d", 
                                fd, fd_evt_driver->evts_capcity);
                return YF_ERROR;
        }

        yf_fd_evt_in_t* alloc_evt = fd_evt_driver->pevents + fd;
        if (alloc_evt->use_flag)
        {
                yf_log_error(YF_LOG_ERR, log, 0, "fd=%d still in use, evts_capcity=%d", 
                                fd, fd_evt_driver->evts_capcity);
                return YF_ERROR;
        }

        alloc_evt->use_flag = 1;
        fd_evt_driver->evts_num++;
        
        yf_memzero(alloc_evt, sizeof(yf_fd_evt_in_t));
        yf_list_add_tail(&alloc_evt->link, &fd_evt_driver->all_evt_list);

        *read = &alloc_evt->read.evt;
        *write = &alloc_evt->write.evt;

        alloc_evt->read.evt.fd = fd;
        alloc_evt->read.evt.type = YF_REVT;
        alloc_evt->read.evt.log = log;
        alloc_evt->read.evt.driver = driver;
        alloc_evt->read.index = YF_INVALID_INDEX;
        alloc_evt->write.evt.fd = fd;
        alloc_evt->write.evt.type = YF_WEVT;
        alloc_evt->write.evt.log = log;
        alloc_evt->write.evt.driver = driver;
        alloc_evt->write.evt.ready = 1;
        alloc_evt->write.index = YF_INVALID_INDEX;

        if (fd_evt_driver->evt_poll->poll_cls->actions.add &&
                fd_evt_driver->evt_poll->poll_cls->actions.add(
                        fd_evt_driver->evt_poll, alloc_evt) != YF_OK)
        {
                yf_log_error(YF_LOG_ERR, log, 0, "fd=%d add poll failed", fd);
                
                yf_free_fd_evt(*read, *write);
                return YF_ERROR;
        }

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "alloc fd evt, fd=%d", fd);

        return YF_OK;
}


yf_int_t  yf_free_fd_evt(yf_fd_event_t* pread, yf_fd_event_t* pwrite)
{
        yf_fd_evt_driver_in_t* fd_evt_driver = &((yf_evt_driver_in_t*)pread->driver)->fd_driver;

        yf_fd_evt_in_t* alloc_evt = container_of(pread, yf_fd_evt_in_t, read.evt);

        yf_fd_event_t* write_check = &alloc_evt->write.evt;
        CHECK_RV(write_check != pwrite, YF_ERROR);

        yf_log_t* log = pread->log;

        if (!alloc_evt->use_flag)
        {
                yf_log_error(YF_LOG_ERR, log, 0, "fd=%d not in use, evts_capcity=%d", 
                                pread->fd, fd_evt_driver->evts_capcity);
                return YF_ERROR;
        }

        alloc_evt->use_flag = 0;

        yf_unregister_fd_evt(pread);
        yf_unregister_fd_evt(pwrite);

        if (fd_evt_driver->evt_poll->poll_cls->actions.del
                && fd_evt_driver->evt_poll->poll_cls->actions.del(
                        fd_evt_driver->evt_poll, alloc_evt) != YF_OK)
        {
                yf_log_error(YF_LOG_ERR, log, 0, "fd=%d del poll failed", 
                                pread->fd);
        }

        yf_log_debug1(YF_LOG_DEBUG, log, 0, "free fd evt, fd=%d", pread->fd);

        pread->fd = YF_INVALID_FD;
        pwrite->fd = YF_INVALID_FD;
        
        fd_evt_driver->evts_num--;
        return YF_OK;
}


yf_int_t  yf_register_fd_evt(yf_fd_event_t* pevent, yf_time_t  *time_out)
{
        yf_fd_evt_driver_in_t* fd_evt_driver = &((yf_evt_driver_in_t*)pevent->driver)->fd_driver;
        yf_fd_evt_link_t* iner_evt = container_of(pevent, yf_fd_evt_link_t, evt);

        pevent->timeout = 0;

        //if ready, no need to poll
        if (pevent->ready)
        {
                yf_log_debug2(YF_LOG_DEBUG, pevent->log, 0, "fd=%d, evt=[%V] "
                                "already ready, no need to poll", 
                                pevent->fd, &yf_evt_tn(pevent));
                
                yf_list_move_head(&iner_evt->ready_linker, &fd_evt_driver->ready_list);
                goto  sucess;
        }
        
        if (iner_evt->active)
                goto sucess;

        if (!iner_evt->polled && fd_evt_driver->evt_poll->poll_cls->actions.activate)
        {
                if (fd_evt_driver->evt_poll->poll_cls->actions.activate(
                        fd_evt_driver->evt_poll, iner_evt) != YF_OK)
                {
                        yf_log_error(YF_LOG_ERR, pevent->log, 0, "fd=%d, evt=%V activate poll failed", 
                                        pevent->fd, &yf_evt_tn(pevent));    
                        return  YF_ERROR;
                }
        }

        if (time_out)
        {
                yf_log_debug3(YF_LOG_DEBUG, pevent->log, 0, "register time-fd evt, fd=%d"
                                ", evt=%V, timeout=%d", 
                                pevent->fd, &yf_evt_tn(pevent), 
                                time_out->tv_sec);
                yf_fd_evt_timer_ctl(pevent, FD_TIMER_NEW, time_out);
        }
        else {
                yf_log_debug2(YF_LOG_DEBUG, pevent->log, 0, 
                                "register fd evt, fd=%d, evt=%V", 
                                pevent->fd, &yf_evt_tn(pevent));
        }

sucess:
        iner_evt->last_active_op_index = fd_evt_driver->active_op_index;
        iner_evt->active = 1;
        return  YF_OK;
}


yf_int_t  yf_unregister_fd_evt(yf_fd_event_t* pevent)
{
        yf_fd_evt_driver_in_t* fd_evt_driver = &((yf_evt_driver_in_t*)pevent->driver)->fd_driver;
        yf_fd_evt_link_t* iner_evt = container_of(pevent, yf_fd_evt_link_t, evt);
        
        if (!iner_evt->active)
                return YF_OK;

        yf_list_del(&iner_evt->ready_linker);
        yf_list_del(&iner_evt->active_linker);

        yf_fd_evt_timer_ctl(pevent, FD_TIMER_DEL, NULL);

        //if not ready, then will poll still... untill ready
        if (pevent->ready && iner_evt->polled && 
                        fd_evt_driver->evt_poll->poll_cls->actions.deactivate)
        {
                if (fd_evt_driver->evt_poll->poll_cls->actions.deactivate(
                        fd_evt_driver->evt_poll, iner_evt) != YF_OK)
                {
                        yf_log_error(YF_LOG_ERR, pevent->log, 0, "fd=%d, evt=%V "
                                        " deactivate poll failed", 
                                        pevent->fd, &yf_evt_type_n[pevent->type]);    
                        return  YF_ERROR;                        
                }
        }

        iner_evt->last_active_op_index = 0;
        iner_evt->active = 0;
        return  YF_OK;
}


yf_int_t  yf_fd_evt_timer_ctl(yf_fd_event_t* pevent
                , int mode, yf_time_t  *time_out)
{
        yf_tm_evt_driver_in_t* tm_evt_driver 
                        = &((yf_evt_driver_in_t*)pevent->driver)->tm_driver;
        yf_fd_evt_link_t* iner_evt = container_of(pevent, yf_fd_evt_link_t, evt);
        
        switch (mode)
        {
                case FD_TIMER_NEW:
                {
                        yf_set_timer_val(iner_evt, *time_out, 1);
                        yf_add_timer(tm_evt_driver, &iner_evt->timer, pevent->log);
                        break;
                }

                case FD_TIMER_DEL:
                {
                        if (iner_evt->timeset) {
                                yf_del_timer(tm_evt_driver, &iner_evt->timer, pevent->log);
                                iner_evt->timeset = 0;
                        }
                }
        }

        return  YF_OK;
}

