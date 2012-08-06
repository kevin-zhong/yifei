#ifndef  _YF_NET_H__
#define _YF_NET_H__

typedef struct yf_connection_s  yf_connection_t;
typedef struct yf_listening_s yf_listening_t;
typedef struct yf_net_s yf_net_t;

#include "yf_net_imp.h"
#include "yf_connection.h"

yf_int_t  yf_init_net(yf_net_t* net, yf_u32_t listen_num, yf_u32_t nfds
                , yf_evt_driver_t *evt_driver, yf_pool_t* pool, yf_log_t  *log);
yf_int_t  yf_uinit_net(yf_net_t* net);


#endif

