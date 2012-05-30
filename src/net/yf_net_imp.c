#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <net/yf_net.h>

yf_int_t  yf_init_net(yf_net_t* net, yf_u32_t listen_num, yf_u32_t nfds
                , yf_evt_driver_t *evt_driver, yf_pool_t* pool, yf_log_t  *log)
{
        net->evt_driver = evt_driver;
        net->log = log;
        
        if (pool == NULL)
        {
                net->pool_self = 1;
                pool = yf_create_pool(YF_NET_POOL_SIZE, log);
                if (pool == NULL)
                        return YF_ERROR;
        }
        else {
                net->pool_self = 0;
        }
        net->pool = pool;
        
        CHECK_OK(yf_array_init(&net->listening, net->pool, 
                        sizeof(yf_listening_t), listen_num));

        net->conn_capcity = nfds;
        net->pconn = yf_palloc(pool, nfds * sizeof(yf_connection_t));
        if (net->pconn == NULL)
                return  YF_ERROR;

        return  YF_OK;
}

yf_int_t  yf_uinit_net(yf_net_t* net)
{
        if (net->pool_self)
        {
                yf_destroy_pool(net->pool);
                net->pool = NULL;
        }
        return YF_OK;
}

