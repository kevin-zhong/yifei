#include <sys/epoll.h>
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include "../event_in/yf_event_base_in.h"

#ifdef  HAVE_SYS_EPOLL_H

typedef  struct
{
        yf_uint_t      nfds;
        yf_uint_t      maxfds;
        yf_int_t        kpfd;
        
        struct epoll_event *event_list;
}
yf_epoll_ctx_t;


static yf_fd_poll_t *yf_epoll_init(yf_u32_t maxfds);
static yf_int_t yf_epoll_uninit(yf_fd_poll_t *epoller);

static yf_int_t yf_epoll_del(yf_fd_poll_t *epoller, yf_fd_evt_in_t *fd_evt);

static yf_int_t yf_epoll_activate(yf_fd_poll_t *epoller, yf_fd_evt_link_t *evt);
static yf_int_t yf_epoll_deactivate(yf_fd_poll_t *epoller, yf_fd_evt_link_t *evt);

static yf_int_t yf_epoll_dispatch(yf_fd_poll_t *epoller);

yf_fd_poll_cls_t yf_fd_epoll_poller_cls =
{
        "epoll",
        {
                yf_epoll_init,
                yf_epoll_uninit,
                NULL,
                yf_epoll_del,
                yf_epoll_activate,
                NULL,
                yf_epoll_dispatch
        }
};


static yf_fd_poll_t *yf_epoll_init(yf_u32_t maxfds)
{
        size_t total_size = yf_align_mem(sizeof(yf_fd_poll_t))
                            + yf_align_mem(sizeof(yf_epoll_ctx_t))
                            + yf_align_mem(sizeof(struct epoll_event) * maxfds);

        yf_fd_poll_t *new_epoller = yf_alloc(total_size);

        if (new_epoller == NULL)
                return NULL;

        yf_memzero(new_epoller, total_size);
        new_epoller->agen_data = (char *)new_epoller + 
                        yf_align_mem(sizeof(yf_fd_poll_t));

        yf_epoll_ctx_t *epoll_ctx = new_epoller->agen_data;

        epoll_ctx->event_list = (struct epoll_event *)((char *)epoll_ctx + 
                        yf_align_mem(sizeof(yf_epoll_ctx_t)));
        epoll_ctx->maxfds = maxfds;

        epoll_ctx->kpfd = epoll_create(maxfds);
        if (epoll_ctx->kpfd < 0) {
                yf_free(new_epoller);
                return  NULL;
        }
        return  new_epoller;
}


static yf_int_t yf_epoll_uninit(yf_fd_poll_t *epoller)
{
        yf_epoll_ctx_t *epoll_ctx = epoller->ctx->evt_poll->agen_data;
        if (epoll_ctx->kpfd >= 0) {
                yf_close(epoll_ctx->kpfd);
                epoll_ctx->kpfd = -1;
        }
        yf_free(epoller);
        return YF_OK;
}

static yf_int_t yf_epoll_del(yf_fd_poll_t *epoller, yf_fd_evt_in_t *fd_evt)
{
        yf_epoll_ctx_t *epoll_ctx = epoller->ctx->evt_poll->agen_data;
        int op;
        struct epoll_event  ee;
        
        if (fd_evt->read.polled || fd_evt->write.polled)
        {
                fd_evt->read.polled = 0;
                fd_evt->write.polled = 0;
                epoll_ctx->nfds--;
                
                op = EPOLL_CTL_DEL;
                ee.events = 0;
                ee.data.ptr = NULL;

                if (epoll_ctl(epoll_ctx->kpfd, op, fd_evt->read.evt.fd, &ee) == -1) 
                {
                        yf_log_error(YF_LOG_ERR, fd_evt->read.evt.log, yf_errno,
                                        "epoll_ctl del(%d, %d) failed", op, 
                                        fd_evt->read.evt.fd);
                        
                        return YF_ERROR;
                }
        }
        return YF_OK;
}


static yf_int_t yf_epoll_activate(yf_fd_poll_t *epoller, yf_fd_evt_link_t *ev)
{
        int op;
        yf_int_t event, pre;
        yf_epoll_ctx_t *epoll_ctx = epoller->ctx->evt_poll->agen_data;
        yf_fd_evt_link_t *eo_part;
        yf_fd_event_t *fd_evt = &ev->evt;
        struct epoll_event  ee;

        if (ev->polled)
                return  YF_OK;

        if (fd_evt->type == YF_REVT)
        {
                eo_part = &container_of(ev, yf_fd_evt_in_t, read)->write;
                event = EPOLLIN;
                pre = EPOLLOUT;
        }
        else {
                eo_part = &container_of(ev, yf_fd_evt_in_t, write)->read;
                event = EPOLLOUT;
                pre = EPOLLIN;
        }

        yf_log_debug3(YF_LOG_DEBUG, ev->evt.log, 0,
                      "epoll add event: fd:%d ev:%i, other polled=%i", 
                      fd_evt->fd, event, eo_part->polled);

        if (eo_part->polled)
        {
                op = EPOLL_CTL_MOD;
                event |= pre;
        }
        else {
                op = EPOLL_CTL_ADD;
                epoll_ctx->nfds++;
        }

        ee.data.fd = fd_evt->fd;
        ee.events = event | EPOLLET;

        if (epoll_ctl(epoll_ctx->kpfd, op, fd_evt->fd, &ee) < 0)
        { 
                yf_log_error(YF_LOG_ERR, ev->evt.log, 
                                yf_errno, "epoll ctl fd failed, fd=%d", fd_evt->fd);
                return  YF_ERROR;
        }

        ev->polled = 1;
        return YF_OK;
}


static yf_int_t yf_epoll_dispatch(yf_fd_poll_t *epoller)
{
        int ready, revents;
        yf_err_t err;
        yf_int_t i, nready;
        yf_uint_t found, level;
        yf_socket_t  fd;

        yf_fd_evt_link_t *link_evt;
        yf_fd_evt_in_t *fd_evt;

        yf_fd_evt_driver_in_t *driver_ctx = epoller->ctx;
        yf_epoll_ctx_t *epoll_ctx = driver_ctx->evt_poll->agen_data;
        yf_evt_driver_in_t *evt_driver = container_of(driver_ctx, yf_evt_driver_in_t, fd_driver);

        yf_log_debug1(YF_LOG_DEBUG, epoller->log, 0, "epoll timer: %ud",
                      evt_driver->tm_driver.nearest_timeout_ms);

        ready = epoll_wait(epoll_ctx->kpfd, epoll_ctx->event_list, epoll_ctx->nfds,
                     (int)evt_driver->tm_driver.nearest_timeout_ms);

        err = (ready == -1) ? yf_errno : 0;

        yf_log_debug2(YF_LOG_DEBUG, epoller->log, 0,
                      "epoll ready %d of %d", ready, epoll_ctx->nfds);

        if (err)
        {
                if (err == YF_EINTR)
                        level = YF_LOG_INFO;
                else {
                        level = YF_LOG_ALERT;
                        if (err == YF_EINVAL)
                                yf_msleep(20);
                }

                yf_log_error(level, epoller->log, err, "epoll() failed");
                return YF_ERROR;
        }

        if (ready == 0)
        {
                yf_log_debug0(YF_LOG_DEBUG, epoller->log, 0,
                             "epoll() returned no events with timeout");
                return YF_ERROR;
        }

        nready = 0;

        for (i = 0; i < ready; i++)
        {
                revents = epoll_ctx->event_list[i].events;
                fd = epoll_ctx->event_list[i].data.fd;

#if 1
                yf_log_debug3(YF_LOG_DEBUG, epoller->log, 0,
                              "epoll: %d: fd:%d rev:%04Xd",
                              i, fd, revents);
#else
                if (revents)
                {
                        yf_log_debug4(YF_LOG_DEBUG, epoller->log, 0,
                                      "epoll: %d: fd:%d ev:%04Xd rev:%04Xd",
                                      i, epoll_ctx->event_list[i].fd, epoll_ctx->event_list[i].events, revents);
                }
#endif

                if (revents & (EPOLLERR|EPOLLHUP)) {
                        yf_log_debug2(YF_LOG_DEBUG, epoller->log, 0,
                                "epoll_wait() error on fd:%d ev:%04XD",
                                fd, revents);
                }
                
                fd_evt = driver_ctx->pevents + fd;

                if (fd_evt->read.evt.fd == -1)
                {
                        yf_log_error(YF_LOG_ALERT, epoller->log, 0, "unexpected event");
                        /*
                         * it is certainly our fault and it should be investigated,
                         * in the meantime we disable this event to avoid a CPU spinning
                         */
                        continue;
                }

                if ((revents & (EPOLLERR|EPOLLHUP))
                        && (revents & (EPOLLIN|EPOLLOUT)) == 0)
                {
                        /*
                        * if the error events were returned without EPOLLIN or EPOLLOUT,
                        * then add these flags to handle the events at least in one
                        * active handler
                        */
                        revents |= EPOLLIN|EPOLLOUT;
                }                

                found = 0;

                if (revents & EPOLLIN)
                {
                        found = 1;
                        link_evt = &fd_evt->read;
                        yf_list_add_tail(&link_evt->ready_linker, &epoller->ctx->ready_list);
                }
                if (revents & EPOLLOUT)
                {
                        found = 1;
                        link_evt = &fd_evt->write;
                        yf_list_add_tail(&link_evt->ready_linker, &epoller->ctx->ready_list);
                }

                if (found)
                {
                        nready++;
                        continue;
                }
        }

        if (ready != nready)
        {
                yf_log_error(YF_LOG_ALERT, epoller->log, 0,
                              "epoll ready != events: %d:%d", ready, nready);
        }

        return YF_OK;
}

#endif
