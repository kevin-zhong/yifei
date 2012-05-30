#ifndef _YF_LOG_H
#define _YF_LOG_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>
#include <ppc/yf_atomic.h>
#include <ppc/yf_fast_lock.h>

#define YF_LOG_EMERG             1
#define YF_LOG_ALERT             2
#define YF_LOG_CRIT              3
#define YF_LOG_ERR               4
#define YF_LOG_WARN              5
#define YF_LOG_NOTICE            6
#define YF_LOG_INFO              7
#define YF_LOG_DEBUG             8

struct yf_log_s
{
        yf_uint_t  log_level;
        yf_file_t*  file;
        char*        log_buf;
        yf_uint_t  max_log_size;
        yf_uint_t  switch_log_size;
        yf_uint_t  recur_log_num;
        yf_uint_t  stat_file_interval;
        yf_lock_t  lock;
};


/*********************************/

#if defined HAVE_C99_VARIADIC_MACROS

#define YF_HAVE_VARIADIC_MACROS  1

#define yf_log_error(level, log, ...)                                        \
        if ((log)->log_level >= level) yf_log_error_core(level, log, __FILE__, __LINE__, __VA_ARGS__)

void yf_log_error_core(yf_uint_t level, yf_log_t *log, const char* _file_, int _line_
                , yf_err_t err, const char *fmt, ...);

#define yf_log_debug(level, log, ...)                                        \
        if ((log)->log_level & level)                                             \
                yf_log_error_core(YF_LOG_DEBUG, log, __FILE__, __LINE__, __VA_ARGS__)

/*********************************/

#elif defined HAVE_GCC_VARIADIC_MACROS

#define YF_HAVE_VARIADIC_MACROS  1

#define yf_log_error(level, log, args...)                                    \
        if ((log)->log_level >= level) yf_log_error_core(level, log, __FILE__, __LINE__, args)

void yf_log_error_core(yf_uint_t level, yf_log_t *log, const char* _file_, int _line_
                , yf_err_t err, const char *fmt, ...);

#define yf_log_debug(level, log, args ...)                                    \
        if ((log)->log_level & level)                                             \
                yf_log_error_core(YF_LOG_DEBUG, log, __FILE__, __LINE__, args)

/*********************************/

#else /* NO VARIADIC MACROS */

#warning  "NO VARIADIC MACROS, so yf_log_error is empty.."

#define YF_HAVE_VARIADIC_MACROS  0

void  yf_log_error(yf_uint_t level, yf_log_t *log, yf_err_t err, const char *fmt, ...);
void yf_log_error_core(yf_uint_t level, yf_log_t *log, const char* _file_, int _line_
                , yf_err_t err, const char *fmt, va_list args);
void  yf_log_debug_core(yf_log_t *log, yf_err_t err, const char *fmt, ...);

#endif /* VARIADIC MACROS */


/*********************************/

#ifdef YF_DEBUG

#if (YF_HAVE_VARIADIC_MACROS)

#define yf_log_debug0  yf_log_debug
#define yf_log_debug1  yf_log_debug
#define yf_log_debug2  yf_log_debug
#define yf_log_debug3  yf_log_debug
#define yf_log_debug4  yf_log_debug
#define yf_log_debug5  yf_log_debug
#define yf_log_debug6  yf_log_debug
#define yf_log_debug7  yf_log_debug
#define yf_log_debug8  yf_log_debug


#else /* NO VARIADIC MACROS */

#define yf_log_debug0(level, log, err, fmt)                                  \
        if ((log)->log_level & level)                                             \
                yf_log_debug_core(log, err, fmt)

#define yf_log_debug1(level, log, err, fmt, arg1)                            \
        if ((log)->log_level & level)                                             \
                yf_log_debug_core(log, err, fmt, arg1)

#define yf_log_debug2(level, log, err, fmt, arg1, arg2)                      \
        if ((log)->log_level & level)                                             \
                yf_log_debug_core(log, err, fmt, arg1, arg2)

#define yf_log_debug3(level, log, err, fmt, arg1, arg2, arg3)                \
        if ((log)->log_level & level)                                             \
                yf_log_debug_core(log, err, fmt, arg1, arg2, arg3)

#define yf_log_debug4(level, log, err, fmt, arg1, arg2, arg3, arg4)          \
        if ((log)->log_level & level)                                             \
                yf_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4)

#define yf_log_debug5(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5)    \
        if ((log)->log_level & level)                                             \
                yf_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4, arg5)

#define yf_log_debug6(level, log, err, fmt,                                  \
                      arg1, arg2, arg3, arg4, arg5, arg6)                    \
        if ((log)->log_level & level)                                             \
                yf_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6)

#define yf_log_debug7(level, log, err, fmt,                                  \
                      arg1, arg2, arg3, arg4, arg5, arg6, arg7)              \
        if ((log)->log_level & level)                                             \
                yf_log_debug_core(log, err, fmt,                                     \
                                  arg1, arg2, arg3, arg4, arg5, arg6, arg7)

#define yf_log_debug8(level, log, err, fmt,                                  \
                      arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)        \
        if ((log)->log_level & level)                                             \
                yf_log_debug_core(log, err, fmt,                                     \
                                  arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)

#endif

#else /* NO YF_DEBUG */

#define yf_log_debug0(level, log, err, fmt)
#define yf_log_debug1(level, log, err, fmt, arg1)
#define yf_log_debug2(level, log, err, fmt, arg1, arg2)
#define yf_log_debug3(level, log, err, fmt, arg1, arg2, arg3)
#define yf_log_debug4(level, log, err, fmt, arg1, arg2, arg3, arg4)
#define yf_log_debug5(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5)
#define yf_log_debug6(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6)
#define yf_log_debug7(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5,    \
                      arg6, arg7)
#define yf_log_debug8(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5,    \
                      arg6, arg7, arg8)

#endif

/*********************************/

yf_log_t *yf_log_open(char *path, yf_log_t* yf_log);

yf_int_t  yf_log_rotate(yf_log_t* yf_log);
void  yf_log_close(yf_log_t* yf_log);
char *yf_log_errno(char *buf, char *last, yf_err_t err);


#endif
