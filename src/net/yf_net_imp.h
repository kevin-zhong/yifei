#ifndef  _YF_NET_IMP_H
#define _YF_NET_IMP_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>
#include <mio_driver/yf_event.h>
#include <net/yf_net.h>

#define YF_NET_POOL_SIZE     16384

struct  yf_net_s
{
        yf_array_t  listening;
        yf_connection_t  *pconn;
        yf_u32_t  conn_capcity;
        yf_u32_t  used_conn;
        yf_pool_t *pool;
        yf_log_t   *log;

        yf_evt_driver_t *evt_driver;
        
        yf_int_t    pool_self:1;
};


#endif

