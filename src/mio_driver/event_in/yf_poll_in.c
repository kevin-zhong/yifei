#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include "yf_event_base_in.h"

#ifdef  HAVE_SYS_SELECT_H
extern  yf_fd_poll_cls_t yf_fd_select_poller_cls;
#endif
#ifdef  HAVE_POLL_H
extern  yf_fd_poll_cls_t yf_fd_poll_poller_cls;
#endif
#ifdef  HAVE_SYS_EPOLL_H
extern  yf_fd_poll_cls_t yf_fd_epoll_poller_cls;
#endif

#define  YF_NONE_POLLER {NULL, {NULL, NULL, NULL, NULL, NULL, NULL, NULL}}

yf_fd_poll_cls_t  yf_fd_poller_cls[] = {
        YF_NONE_POLLER,
        YF_NONE_POLLER,
        YF_NONE_POLLER,
        YF_NONE_POLLER
};

static  yf_int_t  yf_fd_pollers_loaded = 0;

void  yf_load_fd_pollers()
{
        if (yf_fd_pollers_loaded)
                return;

        yf_fd_poll_cls_t  yf_fd_poller_cls_tmp[] = 
        {
                YF_NONE_POLLER,

#ifndef  HAVE_SYS_SELECT_H
                YF_NONE_POLLER,
#else
                yf_fd_select_poller_cls,
#endif

#ifndef  HAVE_POLL_H
                YF_NONE_POLLER,
#else
                yf_fd_poll_poller_cls,
#endif

#ifndef  HAVE_SYS_EPOLL_H
                YF_NONE_POLLER
#else
                yf_fd_epoll_poller_cls
#endif
        };

       yf_memcpy(yf_fd_poller_cls, yf_fd_poller_cls_tmp, sizeof(yf_fd_poller_cls));
}

yf_uint_t  yf_fd_poller_cls_num = YF_ARRAY_SIZE(yf_fd_poller_cls);

