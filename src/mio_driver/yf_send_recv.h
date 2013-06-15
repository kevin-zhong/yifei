#ifndef  _YF_SEND_RECV_H
#define _YF_SEND_RECV_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>
#include <mio_driver/yf_event.h>

typedef struct fd_rw_ctx_s
{
        off_t  rw_cnt;
        
        yf_fd_event_t* fd_evt;
        yf_pool_t* pool;
}
fd_rw_ctx_t;

ssize_t yf_unix_recvfrom(fd_rw_ctx_t *ctx, char *buf, size_t size
                , int flags, struct sockaddr *from, socklen_t *fromlen);

ssize_t yf_unix_readv(fd_rw_ctx_t *ctx, const struct iovec *iov, int iovcnt);

ssize_t yf_unix_sendto(fd_rw_ctx_t *ctx, char *buf, size_t size
                , int flags, const struct sockaddr *to, socklen_t tolen);

ssize_t yf_unix_writev(fd_rw_ctx_t *ctx, const struct iovec *iov, int iovcnt);

/*
* limit = 0, then no limit
*/
ssize_t yf_readv_chain(fd_rw_ctx_t *ctx, yf_chain_t *entry);

yf_chain_t *yf_writev_chain(fd_rw_ctx_t *ctx, yf_chain_t *in, off_t limit);

#endif

