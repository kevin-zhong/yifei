#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include "yf_bridge_in.h"

/*
* channel
*/
static void yf_channel_parent_readable(yf_fd_event_t* evt);
static void yf_channel_child_readable(yf_fd_event_t* evt);

yf_int_t  yf_bridge_channel_init(yf_bridge_channel_t* bc
                , yf_bridge_in_t* bridge
                , yf_socket_t* channels, yf_evt_driver_t* evt_driver
                , yf_int_t is_parent, yf_int_t child_no, yf_log_t* log)
{
        yf_fd_t  fd;
        yf_fd_event_t** fd_evts;

        //cause in tt mode, this...
        //yf_memzero(bc, sizeof(yf_bridge_channel_t));
        
        if (channels)
        {
                bc->channels[0] = channels[0];
                bc->channels[1] = channels[1];
                bc->owner = 0;
        }
        else {
                assert(is_parent);
                if (yf_open_channel(bc->channels, 0, 0, 1, log) != YF_OK)
                {
                        yf_log_error(YF_LOG_WARN, log, 0, 
                                        "yf_open_channel() failed while init bridge");
                        return YF_ERROR;
                }
        }

        fd = bc->channels[is_parent?0:1];
        
        if (evt_driver)
        {
                fd_evts = is_parent ? bc->channel_pevts : bc->channel_cevts;
                
                if (yf_nonblocking(fd) != 0)
                {
                        yf_log_error(YF_LOG_WARN, log, yf_errno, 
                                        "yf_nonblocking() failed while init bridge");
                        goto failed;
                }
                
                if (yf_alloc_fd_evt(evt_driver, fd, fd_evts, fd_evts+1, log) != YF_OK)
                {
                        yf_log_error(YF_LOG_WARN, log, 0, 
                                        "yf_alloc_fd_evt() failed while init bridge");
                        goto failed; 
                }

                //no need to monitor write evt
                fd_evts[0]->data = bc;
                fd_evts[0]->fd_evt_handler = is_parent ? yf_channel_parent_readable
                                : yf_channel_child_readable;
                
                if (yf_register_fd_evt(fd_evts[0], NULL) != YF_OK)
                {
                        yf_log_error(YF_LOG_WARN, log, 0, 
                                        "yf_register_fd_evt() failed while init bridge");
                        goto failed;
                }
        }
        else {
                if (yf_blocking(fd) != 0)
                {
                        yf_log_error(YF_LOG_WARN, log, yf_errno, 
                                        "yf_blocking() failed while init bridge");
                        goto failed;                        
                }
                
                if (is_parent)
                        bc->pblocked = 1;
                else
                        bc->cblocked = 1;
        }

        bc->bridge_in = bridge;
        bc->child_no = child_no;
        return YF_OK;

failed:
        if (!channels)
                yf_close_channel(bc->channels, log);
        if (fd_evts[0])
                yf_free_fd_evt(fd_evts[0], fd_evts[1]);
        return YF_ERROR;
}


yf_int_t  yf_bridge_channel_uninit(yf_bridge_channel_t* bc
                , yf_int_t is_parent, yf_log_t* log)
{
        yf_fd_event_t** fd_evts;

        fd_evts = is_parent ? bc->channel_pevts : bc->channel_cevts;
        if (fd_evts[0])
                yf_free_fd_evt(fd_evts[0], fd_evts[1]);

        if (bc->owner)
                yf_close_channel(bc->channels, log);
        
        return YF_OK;
}


yf_int_t  yf_bridge_channel_signal(yf_bridge_channel_t* bc
                , yf_int_t is_parent, yf_log_t* log)
{
        yf_fd_t  fd = bc->channels[is_parent?0:1];
        yf_channel_t  channel = {0};
        
        channel.command = YF_CMD_DATA;
        yf_int_t ret = yf_write_channel(fd, &channel, log);
        
        if (unlikely(ret != YF_OK))
        {
                if (ret != YF_AGAIN)
                {
                        yf_log_error(YF_LOG_ERR, log, 0, "bridge channel signal failed, fd=%d", fd);
                        return  YF_ERROR;
                }
                yf_log_debug1(YF_LOG_DEBUG, log, 0, "bridge channel signal full, fd=%d", fd);
        }
        return YF_OK;
}


yf_int_t  yf_bridge_channel_wait(yf_bridge_channel_t* bc
                , yf_int_t is_parent, yf_log_t* log)
{
        yf_int_t ret;
        yf_fd_t  fd = bc->channels[is_parent?0:1];
        yf_channel_t  channel = {0};

        yf_int_t blocked = is_parent ? bc->pblocked : bc->cblocked;
        yf_int_t tmp_block = 1;

        //read all from channel...
        while (1)
        {
                ret = yf_read_channel(fd, &channel, log);
                
                if (ret < 0)
                {
                        if (ret != YF_AGAIN)
                        {
                                yf_log_error(YF_LOG_ERR, log, 0, 
                                                "bridge channel wait failed, fd=%d", fd);
                                return  YF_ERROR;
                        }
                        yf_log_debug1(YF_LOG_DEBUG, log, 0, 
                                        "bridge channel wait empty, fd=%d", fd);
                        break;
                }
                else if (unlikely(ret == 0))
                {
                        yf_log_debug1(YF_LOG_DEBUG, log, 0, "channel broken, fd=%d", fd);
                        //TODO...
                        break;
                }
                
                yf_log_debug2(YF_LOG_DEBUG, log, 0, 
                                "bridge channel wait a cmd, fd=%d, cmd=%d", 
                                fd, channel.command);
                if (unlikely(channel.command != YF_CMD_DATA))
                        ;//TODO

                if (blocked && tmp_block)
                {
                        if (yf_nonblocking(fd) != 0)
                        {
                                yf_log_error(YF_LOG_WARN, log, yf_errno, 
                                                "yf_nonblocking() failed while wait bridge");
                                break;
                        }
                        tmp_block = 0;
                }
        }

        if (blocked && !tmp_block)
        {
                if (yf_blocking(fd) != 0)
                {
                        yf_log_error(YF_LOG_WARN, log, yf_errno, 
                                        "yf_blocking() failed while wait bridge");
                }
        }
        return YF_OK;
}


//TODO, if channel broken, then...

void yf_channel_parent_readable(yf_fd_event_t* evt)
{
        yf_bridge_channel_t* bc = (yf_bridge_channel_t*)evt->data;

        yf_bridge_channel_wait(bc, 1, evt->log);

        yf_bridge_on_task_res_valiable(bc->bridge_in, bc->child_no, evt->log);

        evt->ready = 0;
        yf_register_fd_evt(evt, NULL);
}

void yf_channel_child_readable(yf_fd_event_t* evt)
{
        yf_bridge_channel_t* bc = (yf_bridge_channel_t*)evt->data;

        yf_bridge_channel_wait(bc, 0, evt->log);

        yf_bridge_on_task_valiable(bc->bridge_in, bc->child_no, evt->log);

        evt->ready = 0;
        yf_register_fd_evt(evt, NULL);        
}

/*
* mutex+cond
*/
yf_int_t  yf_bridge_mutex_init(yf_bridge_mutex_t* bm
                , yf_bridge_in_t* bridge, yf_log_t* log)
{
        bm->wait_num = 0;
        bm->mutex = yf_mutex_init(log);
        bm->cond = yf_cond_init(log);
        if (bm->mutex == NULL || bm->cond == NULL)
                goto  failed;
        
        bm->bridge_in = bridge;

        return YF_OK;
failed:
        if (bm->mutex)
                yf_mutex_destroy(bm->mutex, log);
        if (bm->cond)
                yf_cond_destroy(bm->cond, log);
        return  YF_ERROR;
}


yf_int_t  yf_bridge_mutex_uninit(yf_bridge_mutex_t* bm
                , yf_log_t* log)
{
        if (bm->mutex)
                yf_mutex_destroy(bm->mutex, log);
        if (bm->cond)
                yf_cond_destroy(bm->cond, log);
        return  YF_OK;
}


yf_int_t  yf_bridge_mutex_lock(yf_bridge_mutex_t* bm
                , yf_log_t* log)
{
        yf_mutex_lock(bm->mutex, log);
        return  YF_OK;
}

yf_int_t  yf_bridge_mutex_unlock(yf_bridge_mutex_t* bm
                , yf_log_t* log)
{
        yf_mutex_unlock(bm->mutex, log);
        return  YF_OK;
}


yf_int_t  yf_bridge_mutex_signal(yf_bridge_mutex_t* bm, yf_int_t alreay_locked
                , yf_log_t* log)
{
        if (!alreay_locked)
                yf_mutex_lock(bm->mutex, log);
        
        if (bm->wait_num)
                yf_cond_signal(bm->cond, log);

        if (!alreay_locked)
                yf_mutex_unlock(bm->mutex, log);
        return YF_OK;
}

yf_int_t  yf_bridge_mutex_wait(yf_bridge_mutex_t* bm
                , yf_log_t* log)
{
        yf_mutex_lock(bm->mutex, log);
        
        bm->wait_num++;
        yf_cond_wait(bm->cond, bm->mutex, log);
        bm->wait_num--;
        
        yf_mutex_unlock(bm->mutex, log);
        return YF_OK;
}

