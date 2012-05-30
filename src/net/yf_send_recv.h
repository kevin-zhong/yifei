#ifndef  _YF_SEND_RECV_H
#define _YF_SEND_RECV_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>
#include <net/yf_net.h>

ssize_t yf_unix_recv(yf_connection_t *c, char *buf, size_t size);
ssize_t yf_readv_chain(yf_connection_t *c, yf_chain_t *entry);
ssize_t yf_unix_send(yf_connection_t *c, char *buf, size_t size);
yf_chain_t *yf_writev_chain(yf_connection_t *c, yf_chain_t *in, off_t limit);

#endif

