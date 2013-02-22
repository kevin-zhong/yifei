#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>
#include <log_ext/yf_log_file.h>

typedef struct
{
        yf_log_t log_meta;
        yf_log_file_init_ctx_t init_ctx;
        
        yf_file_t*  file;
        char*        log_buf;
        yf_uint_t  log_offset;

        yf_time_t  last_write_tm;
        yf_lock_t  write_lock;//protect write_buf, write_offset, write_waiting
        char*        write_buf;
        yf_uint_t  write_offset;
        yf_uint_t  write_waiting:1;//if waiting..., cant switch buf or close...
}
_log_file_ctx_t;

static yf_u64_t     _log_file_write_waiting_bits = 0;
static yf_mutex_t* _log_file_write_mutex = NULL;
static yf_cond_t*   _log_file_write_cond = NULL;

static _log_file_ctx_t  _log_file_ctxs[32];
static yf_lock_t  _open_lock;

static yf_int_t  _log_file_rotate(_log_file_ctx_t* log_ctx);


static yf_log_t* _log_file_open(yf_uint_t log_level
                , yf_u32_t log_max_len, void* init_ctx)
{
        yf_log_file_init_ctx_t* log_init_ctx = (yf_log_file_init_ctx_t*)init_ctx;

        if (log_init_ctx->log_cache_size < 8192)
        {
                fprintf(stderr, "log cache size too small, log open failed...\n");
                return NULL;
        }
        
        if (log_max_len< 1024)
                log_max_len = (log_init_ctx->log_cache_size >> 2);
        else if (log_init_ctx->log_cache_size < log_max_len)
        {
                fprintf(stderr, "log cache size=%d < log max len=%d...\n", 
                                log_init_ctx->log_cache_size, 
                                log_max_len);
                return NULL;
        }
        
        yf_lock(&_open_lock);

        yf_int_t i1 = 0;
        for (i1 = 0; i1 < YF_ARRAY_SIZE(_log_file_ctxs); ++i1)
        {
                if (_log_file_ctxs[i1].file == NULL)
                        break;
        }
        if (i1 == YF_ARRAY_SIZE(_log_file_ctxs))
                goto failed;

        _log_file_ctx_t* log_ctx = _log_file_ctxs + i1;

        yf_memzero(log_ctx, sizeof(_log_file_ctx_t));
        log_ctx->init_ctx = *log_init_ctx;
        
        if (log_init_ctx->log_file_path == NULL)
        {
                log_ctx->file = yf_alloc(sizeof(yf_file_t) + 2*log_init_ctx->log_cache_size);
                if (log_ctx->file == NULL)
                        goto failed;

                log_ctx->file->fd = yf_stderr;
                log_ctx->log_buf = (char*)log_ctx->file + sizeof(yf_file_t);
                log_ctx->write_buf = log_ctx->log_buf + log_init_ctx->log_cache_size;
                yf_lock_init(&log_ctx->write_lock);
                
                yf_unlock(&_open_lock);
                return  &log_ctx->log_meta;
        }
        
        size_t  path_len = yf_strlen(log_init_ctx->log_file_path);
        log_ctx->file = yf_alloc(sizeof(yf_file_t) + 2*log_init_ctx->log_cache_size + path_len);
        if (log_ctx->file == NULL)
                goto failed;
        
        log_ctx->file->fd = yf_stderr;

        log_ctx->log_buf = (char*)log_ctx->file + sizeof(yf_file_t);
        log_ctx->write_buf = log_ctx->log_buf + log_init_ctx->log_cache_size;

        log_ctx->file->name.data = log_ctx->log_buf + 2*log_init_ctx->log_cache_size;
        log_ctx->file->name.len = path_len;
        yf_memcpy(log_ctx->file->name.data, log_init_ctx->log_file_path, path_len);
        
        log_ctx->file->fd = yf_open_file(log_init_ctx->log_file_path, YF_FILE_APPEND, 
                        YF_FILE_CREATE_OR_OPEN, 
                        YF_FILE_DEFAULT_ACCESS);

        if (log_ctx->file->fd == YF_INVALID_FILE) 
        {
                log_ctx->file->fd = yf_stderr;
                
                fprintf(stderr, "[alert] could not open error log file: "
                                yf_open_file_n " \"%s\" failed\n", log_init_ctx->log_file_path);
        }

        yf_lock_init(&log_ctx->write_lock);
        
        yf_unlock(&_open_lock);
        return &log_ctx->log_meta;

failed:
        yf_unlock(&_open_lock);
        return NULL;
}

static void _log_file_close(yf_log_t* yf_log)
{
        _log_file_ctx_t* log_ctx = container_of(yf_log, _log_file_ctx_t, log_meta);

        yf_lock(&_open_lock);
        yf_lock(&log_ctx->write_lock);

        while (1)
        {
                //should wait for log flushed to disk, so may wait for a long time...
                if (log_ctx->write_waiting || log_ctx->log_offset)
                {
                        yf_unlock(&log_ctx->write_lock);
                        yf_usleep(200);
                        yf_lock(&log_ctx->write_lock);
                }
                else
                        break;
        }
        
        if (log_ctx->file->fd != YF_INVALID_FILE
                && log_ctx->file->fd != yf_stderr)
        {
                yf_close_file(log_ctx->file->fd);
                log_ctx->file->fd = YF_INVALID_FILE;
        }
        
        yf_unlock(&log_ctx->write_lock);
        yf_lock_destory(&log_ctx->write_lock);

        yf_free(log_ctx->file);
        yf_unlock(&_open_lock);
}

//buf len must>=each_log_max_len
static char* _log_file_alloc_buf(yf_log_t* yf_log)
{
        _log_file_ctx_t* log_ctx = container_of(yf_log, _log_file_ctx_t, log_meta);
        return log_ctx->log_buf + log_ctx->log_offset;
}

static void _log_file_msg(yf_log_t* yf_log, yf_log_msg_t* logmsg)
{
        _log_file_ctx_t* log_ctx = container_of(yf_log, _log_file_ctx_t, log_meta);
        yf_u32_t log_index = log_ctx - _log_file_ctxs;
        assert(log_index < YF_ARRAY_SIZE(_log_file_ctxs));
        
        log_ctx->log_offset += logmsg->log_len;
        if (log_ctx->log_offset + log_ctx->log_meta.each_log_max_len 
                        < log_ctx->init_ctx.log_cache_size)
                        return;

        yf_lock(&log_ctx->write_lock);
        
        yf_uint_t wait_times = 0;
        while (1)
        {
                if (log_ctx->write_waiting)
                {
                        yf_unlock(&log_ctx->write_lock);
                        yf_usleep(200);
                        wait_times++;
                        yf_lock(&log_ctx->write_lock);
                }
                else {
                        log_ctx->write_waiting = 1;
                        break;
                }
        }

        yf_swap2(log_ctx->write_buf, log_ctx->log_buf);
        log_ctx->write_offset = log_ctx->log_offset;
        log_ctx->log_offset = 0;
        
        yf_unlock(&log_ctx->write_lock);

        //notify
        yf_mlock(_log_file_write_mutex);
        yf_u64_t org_bits = _log_file_write_waiting_bits;
        yf_set_bit(_log_file_write_waiting_bits, log_index);
        if (org_bits == 0) {
                yf_cond_signal(_log_file_write_cond, NULL);
        }
        yf_munlock(_log_file_write_mutex);
}

yf_log_actions_t  _log_file_actions = {
        _log_file_open, 
        _log_file_close, 
        _log_file_alloc_buf, 
        _log_file_msg
};

static  void _log_file_write_impl(_log_file_ctx_t* log_ctx)
{
        assert(log_ctx->write_waiting);
        log_ctx->last_write_tm = yf_now_times.clock_time;

        //cause write_waiting=True, so write buf+offset is safe
        yf_write_fd(log_ctx->file->fd, log_ctx->write_buf, log_ctx->write_offset);

        if (log_ctx->file->fd != yf_stderr)
        {
                struct stat  file_stat;
                if (fstat(log_ctx->file->fd, &file_stat) == 0)
                {
                        if (yf_file_size(&file_stat) >= log_ctx->init_ctx.switch_log_size)
                                _log_file_rotate(log_ctx);
                }
        }

        yf_lock(&log_ctx->write_lock);
        log_ctx->write_waiting = 0;
        yf_unlock(&log_ctx->write_lock);
}

yf_thread_value_t _log_file_write_thread(void *arg)
{
        yf_bit_set_t  bit_val;
        yf_set_bits  set_bits;
        yf_u32_t i1 = 0;
        _log_file_ctx_t* log_file_ctx = NULL;
        
        while (1)
        {
                yf_mlock(_log_file_write_mutex);
                if (0 == _log_file_write_waiting_bits)
                {
                        yf_cond_wait(_log_file_write_cond, _log_file_write_mutex, NULL);
                }

                bit_val.bit_64 = _log_file_write_waiting_bits;
                _log_file_write_waiting_bits = 0;
                yf_munlock(_log_file_write_mutex);
                
                yf_get_set_bits(&bit_val, set_bits);

                for (i1 = 0 ; i1 < 32; ++i1)
                {
                        if (yf_index_end(set_bits, i1))
                                break;
                        
                        log_file_ctx = _log_file_ctxs + i1;
                        _log_file_write_impl(log_file_ctx);
                }
        }
        return NULL;
}


void _log_file_exit_handle(void)
{
        yf_int_t  i1 = 0;
        _log_file_ctx_t* log_file_ctx = NULL;
        
        for (i1 = 0; i1 < YF_ARRAY_SIZE(_log_file_ctxs); ++i1)
        {
                log_file_ctx = _log_file_ctxs + i1;
                if (log_file_ctx->file == NULL)
                        continue;

                if (log_file_ctx->write_waiting)
                {
                        yf_write_fd(log_file_ctx->file->fd, log_file_ctx->write_buf, 
                                        log_file_ctx->write_offset);
                        log_file_ctx->write_waiting = 0;
                }
                
                if (log_file_ctx->log_offset)
                {
#ifdef YF_DEBUG
                        fprintf(stdout, "log flush at exit, len=%d\n", 
                                        log_file_ctx->log_offset);
#endif
                        yf_write_fd(log_file_ctx->file->fd, log_file_ctx->log_buf, 
                                        log_file_ctx->log_offset);
                        log_file_ctx->log_offset = 0;
                }
        }
}


yf_tm_evt_t* _log_flush_tm_evt = NULL;
yf_time_t _log_flush_time_interval;


void _log_file_flush_handle(struct yf_tm_evt_s* evt, yf_time_t* start)
{
        yf_int_t  i1 = 0;
        _log_file_ctx_t* log_file_ctx = NULL;
        yf_int_t  need_notify = 0;
        yf_u32_t flush_ms = yf_time_2_ms(&_log_flush_time_interval), diff_ms = 0;

        yf_lock(&_open_lock);
        yf_mlock(_log_file_write_mutex);

        yf_u64_t org_bits = _log_file_write_waiting_bits;

        for (i1 = 0; i1 < YF_ARRAY_SIZE(_log_file_ctxs); ++i1)
        {
                log_file_ctx = _log_file_ctxs + i1;
                if (log_file_ctx->file == NULL)
                        continue;

                yf_lock(&log_file_ctx->log_meta.lock);
                yf_lock(&log_file_ctx->write_lock);

                diff_ms = yf_time_diff_ms(&yf_now_times.clock_time, 
                                        &log_file_ctx->last_write_tm);
                
                if (!log_file_ctx->write_waiting && log_file_ctx->log_offset
                                && diff_ms >= flush_ms)
                {
#ifdef YF_DEBUG
                        fprintf(stdout, "log flush time reached, len=%d\n", 
                                        log_file_ctx->log_offset);
#endif
                        yf_swap2(log_file_ctx->write_buf, log_file_ctx->log_buf);
                        log_file_ctx->write_offset = log_file_ctx->log_offset;
                        log_file_ctx->log_offset = 0;

                        log_file_ctx->write_waiting = 1;

                        yf_set_bit(_log_file_write_waiting_bits, i1);

                        need_notify = 1;
                }

                yf_unlock(&log_file_ctx->write_lock);
                yf_unlock(&log_file_ctx->log_meta.lock);
        }

        if (need_notify && org_bits == 0) {
                yf_cond_signal(_log_file_write_cond, NULL);
        }

        yf_munlock(_log_file_write_mutex);
        yf_unlock(&_open_lock);
        
        yf_int_t ret = yf_register_tm_evt(_log_flush_tm_evt, 
                                        &_log_flush_time_interval);
        assert(ret == YF_OK);
}


yf_int_t yf_log_file_init(yf_log_t* log)
{
        yf_log_actions = _log_file_actions;
        yf_memzero_st(_log_file_ctxs);
        yf_lock_init(&_open_lock);

        _log_file_write_mutex = yf_mutex_init(log);
        _log_file_write_cond = yf_cond_init(log);
        assert(_log_file_write_mutex && _log_file_write_cond);

        yf_int_t ret = yf_atexit_handler_add(_log_file_exit_handle, log);
        CHECK_RV(ret != YF_OK, ret);

        yf_tid_t tid;
        ret = yf_create_thread(&tid, _log_file_write_thread, NULL,  log);
        assert(ret == YF_OK);
        return ret;
}



yf_int_t yf_log_file_flush_drive(yf_evt_driver_t* evt_driver
                , yf_uint_t flush_ms, yf_log_t* log)
{
        flush_ms = yf_max(flush_ms, 3000);

        yf_ms_2_time(flush_ms, &_log_flush_time_interval);
        
        yf_alloc_tm_evt(evt_driver, &_log_flush_tm_evt, log);
        _log_flush_tm_evt->driver = evt_driver;
        _log_flush_tm_evt->log = log;
        _log_flush_tm_evt->timeout_handler = _log_file_flush_handle;
        yf_int_t ret = yf_register_tm_evt(_log_flush_tm_evt, &_log_flush_time_interval);
        assert(ret == YF_OK);
        return YF_OK;
}



static yf_int_t  _log_file_rotate(_log_file_ctx_t* log_ctx)
{
        if (log_ctx->file->fd == YF_INVALID_FILE)
                return YF_ERROR;

        if (log_ctx->file->fd == yf_stderr 
                || log_ctx->init_ctx.recur_log_num <= 1)
                return  YF_OK;

        char path_src[YF_MAX_PATH + 16], path_dst[YF_MAX_PATH + 16];
        char* path_src_end = yf_cpymem(path_src, 
                        log_ctx->file->name.data, log_ctx->file->name.len);
        char* path_dst_end = yf_cpymem(path_dst, 
                        log_ctx->file->name.data, log_ctx->file->name.len);
        
        yf_sprintf(path_src_end, ".%d", log_ctx->init_ctx.recur_log_num - 1);
        if (yf_delete_file(path_src) != 0 && yf_errno != YF_ENOPATH)
        {
                fprintf(stderr, "delete file path failed, path=%s, err=%d\n", 
                                path_src, yf_errno);
                return  YF_ERROR;
        }

        yf_int_t  i;
        for (i = log_ctx->init_ctx.recur_log_num - 2; i >= 0; --i)
        {
                if (i)
                        yf_sprintf(path_src_end, ".%d", i);
                else
                        *path_src_end = 0;

                yf_sprintf(path_dst_end, ".%d", i + 1);

                if (yf_rename_file(path_src, path_dst) != 0 && yf_errno != YF_ENOPATH)
                {
                        fprintf(stderr, "rename file path failed, path_src=%s, err=%d\n", 
                                        path_src, yf_errno);
                        return  YF_ERROR;
                }
        }

        yf_close_file(log_ctx->file->fd);
        log_ctx->file->fd = YF_INVALID_FILE;

        *path_src_end = 0;
        log_ctx->file->fd = yf_open_file(path_src, YF_FILE_APPEND, 
                        YF_FILE_CREATE_OR_OPEN, 
                        YF_FILE_DEFAULT_ACCESS);

        if (log_ctx->file->fd == YF_INVALID_FILE) 
        {
                log_ctx->file->fd = yf_stderr;
                
                fprintf(stderr, "[alert] could not open error log file: "
                                yf_open_file_n " \"%s\" failed, err=%d\n", 
                                path_src, yf_errno);
        }

        return  YF_OK;
}

