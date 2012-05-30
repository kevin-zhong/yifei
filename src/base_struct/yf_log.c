#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

static yf_str_t err_levels[] = {
        yf_null_str,
        yf_str("emerg"),
        yf_str("alert"),
        yf_str("crit"),
        yf_str("error"),
        yf_str("warn"),
        yf_str("notice"),
        yf_str("info"),
        yf_str("debug")
};

#if (YF_HAVE_VARIADIC_MACROS)
void
yf_log_error_core(yf_uint_t level, yf_log_t *log, const char* _file_, int _line_
                , yf_err_t err, const char *fmt, ...)
#else
void
yf_log_error_core(yf_uint_t level, yf_log_t *log, const char* _file_, int _line_
                , yf_err_t err, const char *fmt, va_list args)
#endif
{
#if (YF_HAVE_VARIADIC_MACROS)
        va_list args;
#endif
        char *p, *last, *msg;
        char *errstr = log->log_buf;

        if (log->file == NULL || log->file->fd == YF_INVALID_FILE)
        {
                return;
        }

        yf_lock(&log->lock);

        last = errstr + log->max_log_size;

        yf_memcpy(errstr, yf_log_time.data,
                  yf_log_time.len);

        p = errstr + yf_log_time.len;

        p = yf_snprintf(p, log->max_log_size, " [%s:%d][%V]", 
                        _file_, _line_, 
                        &err_levels[level]);

        /* pid#tid */
        p = yf_snprintf(p, log->max_log_size, "[%P#" YF_TID_T_FMT "]: ",
                        yf_log_pid, yf_log_tid);

        msg = p;

#if (YF_HAVE_VARIADIC_MACROS)

        va_start(args, fmt);
        p = yf_vslprintf(p, last, fmt, args);
        va_end(args);

#else

        p = yf_vslprintf(p, last, fmt, args);

#endif

        if (err)
        {
                p = yf_log_errno(p, last, err);
        }

        if (p > last - YF_LINEFEED_SIZE)
        {
                p = last - YF_LINEFEED_SIZE;
        }

        yf_linefeed(p);

        (void)yf_write_fd(log->file->fd, errstr, p - errstr);

        if (log->stat_file_interval++ >= 512)
        {
                log->stat_file_interval = 0;
                if (log->file->fd != yf_stderr)
                {
                        struct stat  file_stat;
                        if (fstat(log->file->fd, &file_stat) == 0)
                        {
                                if (yf_file_size(&file_stat) >= log->switch_log_size)
                                        yf_log_rotate(log);
                        }
                }
        }

        yf_unlock(&log->lock);
}


#if !(YF_HAVE_VARIADIC_MACROS)

void
yf_log_error(yf_uint_t level, yf_log_t *log, yf_err_t err, const char *fmt, ...)
{
        va_list args;

        if (log->log_level >= level)
        {
                va_start(args, fmt);
                yf_log_error_core(level, log, "", 0, err, fmt, args);
                va_end(args);
        }
}

void
yf_log_debug_core(yf_log_t *log, yf_err_t err, const char *fmt, ...)
{
        va_list  args;

        va_start(args, fmt);
        yf_log_error_core(YF_LOG_DEBUG, log, "", 0, err, fmt, args);
        va_end(args);
}

#endif


char *
yf_log_errno(char *buf, char *last, yf_err_t err)
{
        if (buf > last - 50)
        {
                /* leave a space for an error code */
                buf = last - 50;
                *buf++ = '.';
                *buf++ = '.';
                *buf++ = '.';
        }

        buf = yf_snprintf(buf, last - buf, " (%d: ", err);
        buf = yf_snprintf(buf, last - buf, "%V)", yf_strerror(err));
        
        return buf;
}


yf_log_t *yf_log_open(char *path, yf_log_t* yf_log)
{
        yf_log->log_level = YF_LOG_DEBUG;
        yf_log->stat_file_interval = 0;
        yf_lock_init(&yf_log->lock);
        
        if (path == NULL)
        {
                yf_log->file = yf_alloc(sizeof(yf_file_t) + yf_log->max_log_size);
                if (yf_log->file == NULL)
                        return NULL;

                yf_log->file->fd = yf_stderr;
                yf_log->log_buf = (char*)yf_log->file + sizeof(yf_file_t);
                return  yf_log;
        }
        
        size_t  path_len = yf_strlen(path);
        yf_log->file = yf_alloc(sizeof(yf_file_t) + yf_log->max_log_size + path_len);
        if (yf_log->file == NULL)
                return NULL;
        
        yf_log->file->fd = yf_stderr;

        yf_log->log_buf = (char*)yf_log->file + sizeof(yf_file_t);

        yf_log->file->name.data = yf_log->log_buf + yf_log->max_log_size;
        yf_log->file->name.len = path_len;
        yf_memcpy(yf_log->file->name.data, path, path_len);
        
        yf_log->file->fd = yf_open_file(path, YF_FILE_APPEND, 
                        YF_FILE_CREATE_OR_OPEN, 
                        YF_FILE_DEFAULT_ACCESS);

        if (yf_log->file->fd == YF_INVALID_FILE) 
        {
                yf_log->file->fd = yf_stderr;
                
                yf_log_error(YF_LOG_ERR, yf_log, yf_errno, 
                                "[alert] could not open error log file: "
                                yf_open_file_n " \"%s\" failed", path);
        }        

        return  yf_log;
}


void  yf_log_close(yf_log_t* yf_log)
{
        yf_lock_destory(&yf_log->lock);
        
        if (yf_log->file->fd == YF_INVALID_FILE
                || yf_log->file->fd == yf_stderr)
                return;

        yf_close_file(yf_log->file->fd);
        yf_log->file->fd = YF_INVALID_FILE;
        yf_free(yf_log->file);
}


yf_int_t  yf_log_rotate(yf_log_t* yf_log)
{
        if (yf_log->file->fd == YF_INVALID_FILE)
                return YF_ERROR;

        if (yf_log->file->fd == yf_stderr 
                || yf_log->recur_log_num <= 1)
                return  YF_OK;

        char path_src[YF_MAX_PATH + 16], path_dst[YF_MAX_PATH + 16];
        char* path_src_end = yf_cpymem(path_src, 
                        yf_log->file->name.data, yf_log->file->name.len);
        char* path_dst_end = yf_cpymem(path_dst, 
                        yf_log->file->name.data, yf_log->file->name.len);        
        
        yf_sprintf(path_src_end, ".%d", yf_log->recur_log_num - 1);
        if (yf_delete_file(path_src) != 0 && yf_errno != YF_ENOPATH)
        {
                yf_log_error(YF_LOG_WARN, yf_log, yf_errno, 
                                "delete file path failed, path=%s", 
                                path_src);
                return  YF_ERROR;
        }

        yf_int_t  i;
        for (i = yf_log->recur_log_num - 2; i >= 0; --i)
        {
                if (i)
                        yf_sprintf(path_src_end, ".%d", i);
                else
                        *path_src_end = 0;

                yf_sprintf(path_dst_end, ".%d", i + 1);

                if (yf_rename_file(path_src, path_dst) != 0 && yf_errno != YF_ENOPATH)
                {
                        yf_log_error(YF_LOG_WARN, yf_log, yf_errno, 
                                        "rename file path failed, path_src=%s", 
                                        path_src);
                        return  YF_ERROR;
                }
        }

        yf_close_file(yf_log->file->fd);
        yf_log->file->fd = YF_INVALID_FILE;

        *path_src_end = 0;
        yf_log->file->fd = yf_open_file(path_src, YF_FILE_APPEND, 
                        YF_FILE_CREATE_OR_OPEN, 
                        YF_FILE_DEFAULT_ACCESS);

        if (yf_log->file->fd == YF_INVALID_FILE) 
        {
                yf_log->file->fd = yf_stderr;
                
                yf_log_error(YF_LOG_ERR, yf_log, yf_errno, 
                                "[alert] could not open error log file: "
                                yf_open_file_n " \"%s\" failed", path_src);
        }                

        return  YF_OK;
}

