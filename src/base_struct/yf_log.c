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

typedef struct
{
        yf_file_t*  file;
        char*        log_buf;        
}
_log_default_ctx_t;

static yf_log_t* _log_default_open(yf_uint_t log_level, yf_u32_t log_max_len, void* init_ctx)
{
        char* path = init_ctx;
        yf_u32_t  path_len = 0;
        if (path)
                path_len = yf_strlen(path);
        
        yf_u32_t head_size = yf_align_mem(sizeof(yf_log_t));
        yf_u32_t ctx_size = yf_align_mem(sizeof(_log_default_ctx_t));
        yf_u32_t all_size = head_size + ctx_size 
                               + sizeof(yf_file_t) + log_max_len + path_len;
        
        yf_log_t* log = (yf_log_t*)yf_alloc(all_size);
        CHECK_RV(log==NULL, NULL);

        _log_default_ctx_t* log_ctx = yf_mem_off(log, head_size);
        log->log_ctx = log_ctx;
        log_ctx->file = yf_mem_off(log_ctx, ctx_size);
        log_ctx->log_buf = yf_mem_off(log_ctx->file, sizeof(yf_file_t));
        
        if (path == NULL)
        {
                log_ctx->file->fd = yf_stderr;
                return log;
        }

        log_ctx->file->name.data = log_ctx->log_buf + log_max_len;
        log_ctx->file->name.len = path_len;
        yf_memcpy(log_ctx->file->name.data, path, path_len);
        
        log_ctx->file->fd = yf_open_file(path, YF_FILE_APPEND, 
                        YF_FILE_CREATE_OR_OPEN, 
                        YF_FILE_DEFAULT_ACCESS);

        if (log_ctx->file->fd == YF_INVALID_FILE) 
        {
                log_ctx->file->fd = yf_stderr;
        }
        return  log;
}

static void _log_default_log_close(yf_log_t* yf_log)
{
        _log_default_ctx_t* log_ctx = yf_log->log_ctx;
        
        if (log_ctx->file->fd == YF_INVALID_FILE
                || log_ctx->file->fd == yf_stderr)
        {
                yf_close_file(log_ctx->file->fd);
                return;
        }
        log_ctx->file->fd = YF_INVALID_FILE;
        yf_free(yf_log);
}

static char* _log_default_alloc_buf(yf_log_t* yf_log)
{
        _log_default_ctx_t* log_ctx = yf_log->log_ctx;
        return log_ctx->log_buf;
}

static void _log_default_log_msg(yf_log_t* yf_log, yf_log_msg_t* logmsg)
{
        _log_default_ctx_t* log_ctx = yf_log->log_ctx;
        //printf("log_len=%d\n", logmsg->log_len);
        yf_write_fd(log_ctx->file->fd, logmsg->log_buf, logmsg->log_len);
}

yf_log_actions_t  yf_log_actions = {
        _log_default_open, 
        _log_default_log_close, 
        _log_default_alloc_buf, 
        _log_default_log_msg
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
        if (log == NULL)
                return;
        
#if (YF_HAVE_VARIADIC_MACROS)
        va_list args;
#endif
        char *p, *last;
        yf_log_msg_t  log_msg;
        yf_log_actions_t* log_actions = log->log_actions;

        yf_lock(&log->lock);
        
        char *errstr = log_actions->alloc_buf(log);

        if (errstr == NULL) {
                yf_unlock(&log->lock);
                return;
        }

        log_msg.err = err;
        log_msg.log_buf = errstr;
        log_msg.log_level = level;

        last = errstr + log->each_log_max_len;

        yf_memcpy(errstr, yf_log_time.data, yf_log_time.len);

        p = errstr + yf_log_time.len;

        p = yf_snprintf(p, last - p, " [%s:%d][%V]", 
                        _file_, _line_, 
                        &err_levels[level]);

        /* pid#tid */
        p = yf_snprintf(p, last - p, "[%P#" YF_TID_T_FMT "]: ",
                        yf_log_pid, yf_log_tid);

        log_msg.msg_buf = p;

#if (YF_HAVE_VARIADIC_MACROS)
        va_start(args, fmt);
        p = yf_vslprintf(p, last, fmt, args);
        va_end(args);
#else
        p = yf_vslprintf(p, last, fmt, args);
#endif

        log_msg.msg_len = p - log_msg.msg_buf;

        if (err) {
                p = yf_log_errno(p, last, err);
        }

        if (p > last - YF_LINEFEED_SIZE)
        {
                p = last - YF_LINEFEED_SIZE;
        }

        yf_linefeed(p);
        log_msg.log_len = p - log_msg.log_buf;

        log_actions->log_msg(log, &log_msg);
        
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


yf_log_t *yf_log_open(yf_uint_t log_level, yf_u32_t log_max_len, void* init_ctx)
{
        if (log_max_len < 1024)
        {
                fprintf(stderr, "log cache size too small, log open failed...");
                return NULL;
        }
        
        yf_log_t* yf_log = yf_log_actions.log_open(log_level, log_max_len, init_ctx);
        CHECK_RV(yf_log == NULL, NULL);
        
        yf_log->log_level = log_level;
        yf_log->each_log_max_len = log_max_len;
        yf_lock_init(&yf_log->lock);

        yf_log->log_actions = (void*)&yf_log_actions;

        return  yf_log;
}


void  yf_log_close(yf_log_t* yf_log)
{
        yf_lock_destory(&yf_log->lock);
        ((yf_log_actions_t*)yf_log->log_actions)->log_close(yf_log);
}


