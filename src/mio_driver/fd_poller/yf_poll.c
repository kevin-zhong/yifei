#include <poll.h>
#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include "../yf_event_base_in.h"

#ifdef  HAVE_POLL_H

typedef  struct
{
        yf_uint_t      nevents;

        struct pollfd *event_list;
}
yf_poll_ctx_t;


static yf_fd_poll_t *yf_poll_init(yf_u32_t nfds);
static yf_int_t yf_poll_uninit(yf_fd_poll_t *poller);

static yf_int_t yf_poll_del(yf_fd_poll_t *poller, yf_fd_evt_in_t *fd_evt);

static yf_int_t yf_poll_activate(yf_fd_poll_t *poller, yf_fd_evt_link_t *evt);
static yf_int_t yf_poll_deactivate(yf_fd_poll_t *poller, yf_fd_evt_link_t *evt);

static yf_int_t yf_poll_dispatch(yf_fd_poll_t *poller);

yf_fd_poll_cls_t yf_fd_poll_poller_cls =
{
        "poll",
        {
                yf_poll_init,
                yf_poll_uninit,
                NULL,
                yf_poll_del,
                yf_poll_activate,
                yf_poll_deactivate,
                yf_poll_dispatch
        }
};


static yf_fd_poll_t *yf_poll_init(yf_u32_t nfds)
{
        size_t total_size = sizeof(yf_fd_poll_t)
                            + sizeof(yf_poll_ctx_t)
                            + sizeof(struct pollfd) * nfds;

        yf_fd_poll_t *new_poller = yf_alloc(total_size);

        if (new_poller == NULL)
                return NULL;

        yf_memzero(new_poller, total_size);
        new_poller->agen_data = (char *)new_poller + sizeof(yf_fd_poll_t);

        yf_poll_ctx_t *poll_ctx = new_poller->agen_data;

        poll_ctx->event_list = (struct pollfd*)((char *)poll_ctx + sizeof(yf_poll_ctx_t));

        return  new_poller;
}

static yf_int_t yf_poll_uninit(yf_fd_poll_t *poller)
{
        yf_free(poller);
}

static yf_int_t yf_poll_del(yf_fd_poll_t *poller, yf_fd_evt_in_t *fd_evt)
{
        if (fd_evt->read.polled)
                yf_poll_deactivate(poller, &fd_evt->read);
        if (fd_evt->write.polled)
                yf_poll_deactivate(poller, &fd_evt->write);
}


static yf_int_t yf_poll_activate(yf_fd_poll_t *poller, yf_fd_evt_link_t *ev)
{
        yf_int_t event;
        yf_poll_ctx_t *poll_ctx = poller->ctx->evt_poll->agen_data;
        yf_fd_evt_link_t *eo_part;
        yf_fd_event_t *fd_evt = &ev->evt;

        if (ev->index != YF_INVALID_INDEX)
        {
                yf_log_error(YF_LOG_ALERT, ev->evt.log, 0,
                             "poll event fd:%d is already set", fd_evt->fd);
                return YF_OK;
        }

        if (fd_evt->type == YF_REVT)
        {
                eo_part = &container_of(ev, yf_fd_evt_in_t, read)->write;
                event = POLLIN;
        }
        else {
                eo_part = &container_of(ev, yf_fd_evt_in_t, write)->read;
                event = POLLOUT;
        }

        yf_log_debug2(YF_LOG_DEBUG, ev->evt.log, 0,
                      "poll add event: fd:%d ev:%i", fd_evt->fd, event);

        if (eo_part->index == YF_INVALID_INDEX)
        {
                poll_ctx->event_list[poll_ctx->nevents].fd = fd_evt->fd;
                poll_ctx->event_list[poll_ctx->nevents].events = (short)event;
                poll_ctx->event_list[poll_ctx->nevents].revents = 0;

                ev->index = poll_ctx->nevents;
                poll_ctx->nevents++;

                yf_log_debug2(YF_LOG_DEBUG, ev->evt.log, 0,
                              "poll add event on new index: %i, nevents=%d", ev->index
                              , poll_ctx->nevents);                
        }
        else {
                yf_log_debug1(YF_LOG_DEBUG, ev->evt.log, 0,
                              "poll add event on index: %i", eo_part->index);

                poll_ctx->event_list[eo_part->index].events |= (short)event;
                ev->index = eo_part->index;
        }

        ev->polled = 1;

        return YF_OK;
}


static yf_int_t yf_poll_deactivate(yf_fd_poll_t *poller, yf_fd_evt_link_t *ev)
{
        yf_int_t event;
        yf_fd_evt_driver_in_t *driver_ctx = poller->ctx;
        yf_poll_ctx_t *poll_ctx = poller->ctx->evt_poll->agen_data;
        yf_fd_evt_link_t *eo_part;
        yf_fd_event_t *fd_evt = &ev->evt;

        if (ev->index == YF_INVALID_INDEX)
        {
                yf_log_error(YF_LOG_ALERT, ev->evt.log, 0,
                             "poll event fd:%d ev:%i is already deleted",
                             fd_evt->fd, event);
                return YF_OK;
        }

        if (event == YF_REVT)
        {
                eo_part = &container_of(ev, yf_fd_evt_in_t, read)->write;
                event = POLLIN;
        }
        else {
                eo_part = &container_of(ev, yf_fd_evt_in_t, write)->read;
                event = POLLOUT;
        }

        yf_log_debug2(YF_LOG_DEBUG, ev->evt.log, 0,
                      "poll del event: fd:%d ev:%i", fd_evt->fd, event);

        if (eo_part->index == YF_INVALID_INDEX)
        {
                poll_ctx->nevents--;

                if (ev->index < (yf_uint_t)poll_ctx->nevents)
                {
                        yf_log_debug2(YF_LOG_DEBUG, ev->evt.log, 0,
                                      "index: copy event %ui to %i", poll_ctx->nevents, ev->index);

                        poll_ctx->event_list[ev->index] = poll_ctx->event_list[poll_ctx->nevents];

                        yf_fd_evt_in_t *other_fd_evt = driver_ctx->pevents +
                                        poll_ctx->event_list[poll_ctx->nevents].fd;

                        if (other_fd_evt->read.evt.fd == -1)
                        {
                                yf_log_error(YF_LOG_ALERT, ev->evt.log, 0,
                                             "unexpected last event");
                        }
                        else {
                                if (other_fd_evt->read.index == (yf_uint_t)poll_ctx->nevents)
                                        other_fd_evt->read.index = ev->index;

                                if (other_fd_evt->write.index == (yf_uint_t)poll_ctx->nevents)
                                        other_fd_evt->write.index = ev->index;
                        }
                }
        }
        else {
                yf_log_debug1(YF_LOG_DEBUG, ev->evt.log, 0,
                              "poll del index: %i", eo_part->index);

                poll_ctx->event_list[eo_part->index].events &= (short)~event;
        }

        ev->index = YF_INVALID_INDEX;
        ev->polled = 0;

        return YF_OK;
}


static yf_int_t yf_poll_dispatch(yf_fd_poll_t *poller)
{
        int ready, revents;
        yf_err_t err;
        yf_int_t i, nready;
        yf_uint_t found, level;

        yf_fd_evt_link_t *link_evt;
        yf_fd_evt_in_t *fd_evt;

        yf_fd_evt_driver_in_t *driver_ctx = poller->ctx;
        yf_poll_ctx_t *poll_ctx = driver_ctx->evt_poll->agen_data;
        yf_evt_driver_in_t *evt_driver = container_of(driver_ctx, yf_evt_driver_in_t, fd_driver);

        yf_log_debug1(YF_LOG_DEBUG, poller->log, 0, "poll timer: %ud",
                      evt_driver->tm_driver.nearest_timeout_ms);

        ready = poll(poll_ctx->event_list, poll_ctx->nevents,
                     (int)evt_driver->tm_driver.nearest_timeout_ms);

        err = (ready == -1) ? yf_errno : 0;

        yf_log_debug2(YF_LOG_DEBUG, poller->log, 0,
                      "poll ready %d of %d", ready, poll_ctx->nevents);

        if (err)
        {
                if (err == YF_EINTR)
                        level = YF_LOG_INFO;
                else
                        level = YF_LOG_ALERT;

                yf_log_error(level, poller->log, err, "poll() failed");
                return YF_ERROR;
        }

        if (ready == 0)
        {
                yf_log_error(YF_LOG_ALERT, poller->log, 0,
                             "poll() returned no events without timeout");
                return YF_ERROR;
        }

        nready = 0;

        for (i = 0; i < poll_ctx->nevents && ready; i++)
        {
                revents = poll_ctx->event_list[i].revents;

#if 1
                yf_log_debug4(YF_LOG_DEBUG, poller->log, 0,
                              "poll: %d: fd:%d ev:%04Xd rev:%04Xd",
                              i, poll_ctx->event_list[i].fd, poll_ctx->event_list[i].events, revents);
#else
                if (revents)
                {
                        yf_log_debug4(YF_LOG_DEBUG, poller->log, 0,
                                      "poll: %d: fd:%d ev:%04Xd rev:%04Xd",
                                      i, poll_ctx->event_list[i].fd, poll_ctx->event_list[i].events, revents);
                }
#endif

                if (revents & POLLNVAL)
                {
                        yf_log_error(YF_LOG_ALERT, poller->log, 0,
                                     "poll() error fd:%d ev:%04Xd rev:%04Xd",
                                     poll_ctx->event_list[i].fd, poll_ctx->event_list[i].events, revents);
                }

                if (revents & ~(POLLIN | POLLOUT | POLLERR | POLLHUP | POLLNVAL))
                {
                        yf_log_error(YF_LOG_ALERT, poller->log, 0,
                                     "strange poll() events fd:%d ev:%04Xd rev:%04Xd",
                                     poll_ctx->event_list[i].fd, poll_ctx->event_list[i].events, revents);
                }

                if (poll_ctx->event_list[i].fd == -1)
                        /*
                         * the disabled event, a workaround for our possible bug,
                         * see the comment below
                         */
                        continue;

                fd_evt = driver_ctx->pevents + poll_ctx->event_list[i].fd;

                if (fd_evt->read.evt.fd == -1)
                {
                        yf_log_error(YF_LOG_ALERT, poller->log, 0, "unexpected event");
                        /*
                         * it is certainly our fault and it should be investigated,
                         * in the meantime we disable this event to avoid a CPU spinning
                         */
                        if (i == poll_ctx->nevents - 1)
                                poll_ctx->nevents--;
                        else
                                poll_ctx->event_list[i].fd = -1;

                        continue;
                }

                if ((revents & (POLLERR | POLLHUP | POLLNVAL))
                    && (revents & (POLLIN | POLLOUT)) == 0)
                        /*
                         * if the error events were returned without POLLIN or POLLOUT,
                         * then add these flags to handle the events at least in one
                         * 3 handler
                         */
                        revents |= POLLIN | POLLOUT;

                found = 0;

                if (revents & POLLIN)
                {
                        found = 1;
                        link_evt = &fd_evt->read;
                        yf_list_add_tail(&link_evt->ready_linker, &poller->ctx->ready_list);
                }
                if (revents & POLLOUT)
                {
                        found = 1;
                        link_evt = &fd_evt->write;
                        yf_list_add_tail(&link_evt->ready_linker, &poller->ctx->ready_list);
                }

                if (found)
                {
                        ready--;
                        continue;
                }
        }

        if (ready != 0)
                yf_log_error(YF_LOG_ALERT, poller->log, 0, "poll ready != events");

        return YF_OK;
}

#endif
