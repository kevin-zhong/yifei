#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
#include "yf_send_recv.h"

#ifndef IOV_MAX
#define YF_IOVS  64
#else

#if (IOV_MAX > 64)
#define YF_IOVS  64
#else
#define YF_IOVS  IOV_MAX
#endif

#endif

ssize_t
yf_unix_recv(fd_rw_ctx_t *ctx, char *buf, size_t size)
{
        ssize_t n;
        yf_err_t err;
        yf_fd_event_t *rev;

        rev = ctx->fd_evt;

        do {
                n = recv(rev->fd, buf, size, 0);

                yf_log_debug3(YF_LOG_DEBUG, rev->log, 0,
                               "recv: fd:%d %d of %d", rev->fd, n, size);

                if (n == 0)
                {
                        rev->ready = 0;
                        rev->eof = 1;
                        return n;
                }
                else if (n > 0)
                {
                        if ((size_t)n < size)
                        {
                                rev->ready = 0;
                        }
                        ctx->rw_cnt += n;
                        return n;
                }

                err = yf_socket_errno;

                if (YF_EAGAIN(err) || err == YF_EINTR)
                {
                        yf_log_debug0(YF_LOG_DEBUG, rev->log, err,
                                       "recv() not ready");
                        n = YF_AGAIN;
                }
                else {
                        n = YF_ERROR;
                        yf_log_error(YF_LOG_ERR, rev->log, err, "recv() failed");
                        break;
                }
        } 
        while (err == YF_EINTR);

        rev->ready = 0;

        if (n == YF_ERROR)
        {
                rev->error = 1;
        }

        return n;
}


ssize_t
yf_readv_chain(fd_rw_ctx_t *ctx, yf_chain_t *chain)
{
        char *prev;
        ssize_t n, size;
        yf_err_t err;
        yf_array_t vec;
        yf_fd_event_t *rev;
        struct iovec *iov, iovs[YF_IOVS];

        prev = NULL;
        iov = NULL;
        size = 0;

        vec.elts = iovs;
        vec.nelts = 0;
        vec.size = sizeof(struct iovec);
        vec.nalloc = YF_IOVS;
        vec.pool = ctx->pool;

        rev = ctx->fd_evt;

        /* coalesce the neighbouring bufs */

        while (chain)
        {
                if (prev == chain->buf->last)
                {
                        iov->iov_len += chain->buf->end - chain->buf->last;
                }
                else {
                        iov = yf_array_push(&vec);
                        if (iov == NULL)
                        {
                                return YF_ERROR;
                        }

                        iov->iov_base = (void *)chain->buf->last;
                        iov->iov_len = chain->buf->end - chain->buf->last;
                }

                size += chain->buf->end - chain->buf->last;
                prev = chain->buf->end;
                chain = chain->next;
        }

        yf_log_debug2(YF_LOG_DEBUG, rev->log, 0,
                       "readv: %d:%d", vec.nelts, iov->iov_len);

        do {
                n = readv(rev->fd, (struct iovec *)vec.elts, vec.nelts);

                if (n == 0)
                {
                        rev->ready = 0;
                        rev->eof = 1;

                        return n;
                }
                else if (n > 0)
                {
                        if (n < size)
                        {
                                rev->ready = 0;
                        }

                        ctx->rw_cnt += n;
                        return n;
                }

                err = yf_socket_errno;

                if (YF_EAGAIN(err) || err == YF_EINTR)
                {
                        yf_log_debug0(YF_LOG_DEBUG, rev->log, err,
                                       "readv() not ready");
                        n = YF_AGAIN;
                }
                else {
                        n = YF_ERROR;
                        yf_log_error(YF_LOG_ERR, rev->log, err, "readv() failed");
                        break;
                }
        } 
        while (err == YF_EINTR);

        rev->ready = 0;

        if (n == YF_ERROR)
        {
                rev->error = 1;
        }

        return n;
}

ssize_t
yf_unix_send(fd_rw_ctx_t *ctx, char *buf, size_t size)
{
        ssize_t n;
        yf_err_t err;
        yf_fd_event_t *wev;

        wev = ctx->fd_evt;

        for (;; )
        {
                n = send(wev->fd, buf, size, 0);

                yf_log_debug3(YF_LOG_DEBUG, wev->log, 0,
                               "send: fd:%d %d of %d", wev->fd, n, size);

                if (n > 0)
                {
                        if (n < (ssize_t)size)
                        {
                                wev->ready = 0;
                        }

                        ctx->rw_cnt += n;

                        return n;
                }

                err = yf_socket_errno;

                if (n == 0)
                {
                        yf_log_error(YF_LOG_ALERT, wev->log, err, "send() returned zero");
                        wev->ready = 0;
                        return n;
                }

                if (YF_EAGAIN(err) || err == YF_EINTR)
                {
                        wev->ready = 0;

                        yf_log_debug0(YF_LOG_DEBUG, wev->log, err,
                                       "send() not ready");

                        if (YF_EAGAIN(err))
                        {
                                return YF_AGAIN;
                        }
                }
                else {
                        wev->error = 1;
                        yf_log_error(YF_LOG_ERR, wev->log, err, "send() failed");
                        return YF_ERROR;
                }
        }
}


yf_chain_t *
yf_writev_chain(fd_rw_ctx_t *ctx, yf_chain_t *in, off_t limit)
{
        char *prev;
        ssize_t n, size, sent;
        off_t send, prev_send;
        yf_uint_t eintr, complete;
        yf_err_t err;
        yf_array_t vec;
        yf_chain_t *cl;
        yf_fd_event_t *wev;
        struct iovec *iov, iovs[YF_IOVS];

        wev = ctx->fd_evt;

        if (!wev->ready)
        {
                return in;
        }

        /* the maximum limit size is the maximum size_t value - the page size */

        if (limit == 0 || limit > (off_t)(INT32_MAX - yf_pagesize))
        {
                limit = INT32_MAX - yf_pagesize;
        }

        send = 0;
        complete = 0;

        vec.elts = iovs;
        vec.size = sizeof(struct iovec);
        vec.nalloc = YF_IOVS;
        vec.pool = ctx->pool;

        for (;; )
        {
                prev = NULL;
                iov = NULL;
                eintr = 0;
                prev_send = send;

                vec.nelts = 0;

                /* create the iovec and coalesce the neighbouring bufs */

                for (cl = in; cl && vec.nelts < YF_IOVS && send < limit; cl = cl->next)
                {
                        size = cl->buf->last - cl->buf->pos;

                        if (send + size > limit)
                        {
                                size = (ssize_t)(limit - send);
                        }

                        if (prev == cl->buf->pos)
                        {
                                iov->iov_len += size;
                        }
                        else {
                                iov = yf_array_push(&vec);
                                if (iov == NULL)
                                {
                                        return YF_CHAIN_ERROR;
                                }

                                iov->iov_base = (void *)cl->buf->pos;
                                iov->iov_len = size;
                        }

                        prev = cl->buf->pos + size;
                        send += size;
                }

                n = writev(wev->fd, vec.elts, vec.nelts);

                if (n == -1)
                {
                        err = yf_errno;

                        if (!YF_EAGAIN(err))
                        {
                                switch (err)
                                {
                                case YF_EINTR:
                                        eintr = 1;
                                        break;

                                default:
                                        wev->error = 1;
                                        yf_log_error(YF_LOG_ERR, wev->log, err, "writev() failed");
                                        return YF_CHAIN_ERROR;
                                }
                        }

                        yf_log_debug0(YF_LOG_DEBUG, wev->log, err,
                                       "writev() not ready");
                }

                sent = n > 0 ? n : 0;

                yf_log_debug1(YF_LOG_DEBUG, wev->log, 0, "writev: %z", sent);

                if (send - prev_send == sent)
                {
                        complete = 1;
                }

                ctx->rw_cnt += sent;

                for (cl = in; cl; cl = cl->next)
                {
                        if (sent == 0)
                        {
                                break;
                        }

                        size = cl->buf->last - cl->buf->pos;

                        if (sent >= size)
                        {
                                sent -= size;
                                cl->buf->pos = cl->buf->last;

                                continue;
                        }

                        cl->buf->pos += sent;

                        break;
                }

                if (eintr)
                {
                        continue;
                }

                if (!complete)
                {
                        wev->ready = 0;
                        return cl;
                }

                if (send >= limit || cl == NULL)
                {
                        return cl;
                }

                in = cl;
        }
}
