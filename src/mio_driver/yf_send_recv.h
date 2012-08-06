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

ssize_t yf_unix_recv(fd_rw_ctx_t *ctx, char *buf, size_t size);
ssize_t yf_readv_chain(fd_rw_ctx_t *ctx, yf_chain_t *entry);

ssize_t yf_unix_send(fd_rw_ctx_t *ctx, char *buf, size_t size);
/*
* limit = 0, then no limit
*/
yf_chain_t *yf_writev_chain(fd_rw_ctx_t *ctx, yf_chain_t *in, off_t limit);

#endif

