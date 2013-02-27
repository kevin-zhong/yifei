#include <sys/select.h>
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include "../event_in/yf_event_base_in.h"

#ifdef  HAVE_SYS_SELECT_H

typedef  struct
{
        yf_int_t           max_fd;
        yf_uint_t          nevents;

        yf_fd_evt_link_t **event_index;

        fd_set             read_fd_set;
        fd_set             write_fd_set;
        fd_set             work_read_fd_set;
        fd_set             work_write_fd_set;
}
yf_select_ctx_t;


static yf_fd_poll_t *yf_select_init(yf_u32_t nfds);
static yf_int_t yf_select_uninit(yf_fd_poll_t *poller);

static yf_int_t yf_select_del(yf_fd_poll_t* poller, yf_fd_evt_in_t* fd_evt);

static yf_int_t yf_select_activate(yf_fd_poll_t *poller, yf_fd_evt_link_t *evt);
static yf_int_t yf_select_deactivate(yf_fd_poll_t *poller, yf_fd_evt_link_t *evt);

static yf_int_t yf_select_dispatch(yf_fd_poll_t *poller);
static void yf_select_repair_fd_sets(yf_fd_poll_t *poller);

yf_fd_poll_cls_t yf_fd_select_poller_cls =
{
        "select",
        {
                yf_select_init,
                yf_select_uninit,
                NULL,
                yf_select_del,
                yf_select_activate,
                yf_select_deactivate,
                yf_select_dispatch
        }
};


static yf_fd_poll_t *yf_select_init(yf_u32_t nfds)
{
        size_t  total_size = yf_align_mem(sizeof(yf_fd_poll_t))
                        + yf_align_mem(sizeof(yf_select_ctx_t))
                        + yf_align_mem(sizeof(yf_fd_evt_link_t *) * nfds * 2);
        yf_fd_poll_t *new_poller = yf_alloc(total_size);

        if (new_poller == NULL)
                return NULL;

        yf_memzero(new_poller, total_size);
        new_poller->agen_data = (char *)new_poller + yf_align_mem(sizeof(yf_fd_poll_t));

        yf_select_ctx_t *select_ctx = new_poller->agen_data;

        FD_ZERO(&select_ctx->read_fd_set);
        FD_ZERO(&select_ctx->work_read_fd_set);

        select_ctx->max_fd = -1;

        select_ctx->event_index = (yf_fd_evt_link_t **)((char *)select_ctx + 
                                yf_align_mem(sizeof(yf_select_ctx_t)));

        return  new_poller;
}

static yf_int_t yf_select_uninit(yf_fd_poll_t *poller)
{
        yf_free(poller);
        return YF_OK;
}


static yf_int_t yf_select_activate(yf_fd_poll_t *poller, yf_fd_evt_link_t *evt)
{
        if (evt->index != YF_INVALID_INDEX)
        {
                yf_log_error(YF_LOG_ALERT, evt->evt.log, 0, 
                                "select fd=%d is already set, index=%d", 
                                evt->evt.fd, evt->index);
                return YF_OK;
        }

        yf_select_ctx_t *select_ctx = poller->ctx->evt_poll->agen_data;

        if (evt->evt.type == YF_REVT)
        {
                FD_SET(evt->evt.fd, &select_ctx->read_fd_set);
        }
        else if (evt->evt.type == YF_WEVT)
        {
                FD_SET(evt->evt.fd, &select_ctx->write_fd_set);
        }
        else {
                yf_log_error(YF_LOG_ALERT, evt->evt.log, 0, 
                             "select fd=%d evt type err=%d", evt->evt.fd, evt->evt.type);
                return YF_ERROR;
        }

        if (select_ctx->max_fd != -1 && select_ctx->max_fd < evt->evt.fd)
        {
                select_ctx->max_fd = evt->evt.fd;
        }

        evt->polled = 1;

        select_ctx->event_index[select_ctx->nevents] = evt;
        evt->index = select_ctx->nevents;
        select_ctx->nevents++;

        yf_log_debug4(YF_LOG_DEBUG, evt->evt.log, 0, 
                        "activate evt=%V on fd=%d, nevents=%d, maxfd=%d",
                        &yf_evt_type_n[evt->evt.type], evt->evt.fd,
                        select_ctx->nevents,
                        select_ctx->max_fd);

        return YF_OK;
}


static yf_int_t yf_select_deactivate(yf_fd_poll_t *poller, yf_fd_evt_link_t *evt)
{
        yf_fd_evt_link_t *e = NULL;
        yf_select_ctx_t *select_ctx = poller->ctx->evt_poll->agen_data;

        evt->polled = 0;

        if (evt->index == YF_INVALID_INDEX)
        {
                return YF_OK;
        }

        yf_log_debug1(YF_LOG_DEBUG, evt->evt.log, 0,
                      "select del event fd:%d", evt->evt.fd);

        if (evt->evt.type == YF_REVT)
        {
                FD_CLR(evt->evt.fd, &select_ctx->read_fd_set);
        }
        else if (evt->evt.type == YF_WEVT)
        {
                FD_CLR(evt->evt.fd, &select_ctx->write_fd_set);
        }
        else {
                yf_log_error(YF_LOG_ALERT, evt->evt.log, 0, "select fd=%d evt type err=%d"
                             , evt->evt.fd, evt->evt.type);
                return YF_ERROR;
        }

        if (select_ctx->max_fd == evt->evt.fd)
        {
                select_ctx->max_fd = -1;
        }

        if (evt->index < --select_ctx->nevents)
        {
                e = select_ctx->event_index[select_ctx->nevents];
                select_ctx->event_index[evt->index] = e;
                e->index = evt->index;
        }

        evt->index = YF_INVALID_INDEX;

        return YF_OK;
}


static yf_int_t yf_select_del(yf_fd_poll_t* poller, yf_fd_evt_in_t* fd_evt)
{
        if (fd_evt->read.polled)
                CHECK_OK(yf_select_deactivate(poller, &fd_evt->read));
        if (fd_evt->write.polled)
                CHECK_OK(yf_select_deactivate(poller, &fd_evt->write));

        return YF_OK;
}


static yf_int_t yf_select_dispatch(yf_fd_poll_t *poller)
{
        int ready, nready;
        yf_err_t err;
        yf_uint_t i, found;
        yf_utime_t  tmv;

        yf_fd_evt_link_t *link_evt;
        
        yf_fd_event_t* fd_evt;
        yf_select_ctx_t *select_ctx = poller->ctx->evt_poll->agen_data;
        yf_evt_driver_in_t *evt_driver = container_of(poller->ctx, yf_evt_driver_in_t, fd_driver);

        if (select_ctx->max_fd == -1)
        {
                for (i = 0; i < select_ctx->nevents; i++)
                {
                        fd_evt = &select_ctx->event_index[i]->evt;
                        if (select_ctx->max_fd < fd_evt->fd)
                        {
                                select_ctx->max_fd = fd_evt->fd;
                        }
                }

                yf_log_debug1(YF_LOG_DEBUG, poller->log, 0,
                               "change max_fd: %d", select_ctx->max_fd);
        }

        yf_ms_2_utime(evt_driver->tm_driver.nearest_timeout_ms, &tmv);

        yf_log_debug1(YF_LOG_DEBUG, poller->log, 0,
                       "select timer: %ud", evt_driver->tm_driver.nearest_timeout_ms);

        select_ctx->work_read_fd_set = select_ctx->read_fd_set;
        select_ctx->work_write_fd_set = select_ctx->write_fd_set;

        ready = yf_select(select_ctx->max_fd + 1, 
                        &select_ctx->work_read_fd_set, &select_ctx->work_write_fd_set, 
                        NULL, &tmv);

        err = (ready == -1) ? yf_errno : 0;

        yf_log_debug1(YF_LOG_DEBUG, poller->log, 0,
                       "select ready %d", ready);

        if (err)
        {
                yf_uint_t level;

                if (err == YF_EINTR)
                {
                        level = YF_LOG_INFO;
                }
                else {
                        level = YF_LOG_ALERT;
                        if (err == YF_EINVAL)
                                yf_msleep(20);
                }

                yf_log_error(level, poller->log, err, "select() failed");

                if (err == EBADF)
                {
                        yf_select_repair_fd_sets(poller);
                }

                return YF_ERROR;
        }

        if (ready == 0)
        {
                yf_log_debug0(YF_LOG_DEBUG, poller->log, 0,
                              "select() returned no events with timeout");
                return YF_OK;
        }
        
        nready = 0;

        for (i = 0; i < select_ctx->nevents; i++)
        {
                link_evt = select_ctx->event_index[i];
                fd_evt = &link_evt->evt;
                found = 0;

                if (fd_evt->type == YF_WEVT)
                {
                        if (FD_ISSET(fd_evt->fd, &select_ctx->work_write_fd_set))
                        {
                                found = 1;
                                yf_log_debug1(YF_LOG_DEBUG, poller->log, 0,
                                               "select write %d", fd_evt->fd);
                        }
                }
                else {
                        if (FD_ISSET(fd_evt->fd, &select_ctx->work_read_fd_set))
                        {
                                found = 1;
                                yf_log_debug1(YF_LOG_DEBUG, poller->log, 0,
                                               "select read %d", fd_evt->fd);
                        }
                }

                if (found)
                {
                        yf_list_add_tail(&link_evt->ready_linker, &poller->ctx->ready_list);
                        nready++;
                }
        }
        
        if (ready != nready)
        {
                yf_log_error(YF_LOG_ALERT, poller->log, 0,
                              "select ready != events: %d:%d", ready, nready);

                yf_select_repair_fd_sets(poller);
        }

        return YF_OK;
}


static void yf_select_repair_fd_sets(yf_fd_poll_t *poller)
{
        int n;
        yf_sock_len_t len;
        yf_err_t err;
        yf_socket_t s;

        yf_select_ctx_t *select_ctx = poller->ctx->evt_poll->agen_data;

        for (s = 0; s <= select_ctx->max_fd; s++)
        {
                if (FD_ISSET(s, &select_ctx->read_fd_set) == 0)
                {
                        continue;
                }

                len = sizeof(int);

                if (yf_getsockopt(s, SOL_SOCKET, SO_TYPE, &n, &len) == -1)
                {
                        err = yf_socket_errno;

                        yf_log_error(YF_LOG_ALERT, poller->log, err,
                                     "invalid descriptor #%d in read fd_set", s);

                        FD_CLR(s, &select_ctx->read_fd_set);
                }
        }

        for (s = 0; s <= select_ctx->max_fd; s++)
        {
                if (FD_ISSET(s, &select_ctx->write_fd_set) == 0)
                {
                        continue;
                }

                len = sizeof(int);

                if (yf_getsockopt(s, SOL_SOCKET, SO_TYPE, &n, &len) == -1)
                {
                        err = yf_socket_errno;

                        yf_log_error(YF_LOG_ALERT, poller->log, err,
                                     "invalid descriptor #%d in write fd_set", s);

                        FD_CLR(s, &select_ctx->write_fd_set);
                }
        }

        select_ctx->max_fd = -1;
}

#endif

