#ifndef  _YF_TIME_H
#define  _YF_TIME_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

#ifdef  HAVE_GETTIMEOFDAY
#define yf_utime_t struct timeval
#else
typedef  struct
{
        yf_s32_t  tv_sec;
        yf_s32_t  tv_usec;
} 
yf_utime_t;
#endif

typedef  struct
{
        yf_s32_t  tv_sec;
        yf_s32_t  tv_msec;
} 
yf_time_t;


#define yf_utime_2_ms(tm_val) ((tm_val)->tv_sec * 1000 + (tm_val)->tv_usec / 1000)
#define yf_time_2_ms(tm_val) ((tm_val)->tv_sec * 1000 + (tm_val)->tv_msec)

#define yf_ms_2_utime(tm_val, utime) ({(utime)->tv_sec = (tm_val) / 1000; \
                (utime)->tv_usec = (tm_val) % 1000 * 1000;})
#define yf_ms_2_time(tm_val, utime) ({(utime)->tv_sec = (tm_val) / 1000; \
                (utime)->tv_msec = (tm_val) % 1000;})

#define yf_time_diff_ms(tm1, tm2) (((yf_s64_t)((tm1)->tv_sec - (tm2)->tv_sec)) * 1000 \
                + (tm1)->tv_msec - (tm2)->tv_msec)

#define __yf_time_cmp(tm1, tm2, op, sec_member) (((tm1)->tv_sec op (tm2)->tv_sec) \
                && (!((tm2)->tv_sec op (tm1)->tv_sec) || (tm1)->sec_member op (tm2)->sec_member))

#define yf_utime_cmp(tm1, tm2, op)  __yf_time_cmp(tm1, tm2, op, tv_usec)
#define yf_time_cmp(tm1, tm2, op)  __yf_time_cmp(tm1, tm2, op, tv_msec)


typedef  struct
{
        yf_utime_t  wall_utime;
        yf_utime_t  clock_utime;
        yf_time_t    clock_time;
}
yf_times_t;

typedef struct tm  yf_stm_t;

#define yf_tm_sec            tm_sec
#define yf_tm_min            tm_min
#define yf_tm_hour           tm_hour
#define yf_tm_mday           tm_mday
#define yf_tm_mon            tm_mon
#define yf_tm_year           tm_year
#define yf_tm_wday           tm_wday
#define yf_tm_isdst          tm_isdst

extern  yf_times_t  yf_start_times;

typedef struct
{
        yf_times_t  now_times;
        yf_str_t      log_time;
        yf_utime_t  last_clock_utime;
        yf_utime_t  last_real_wall_utime;
        char           log_buf[sizeof("70/01/01 12:00:00 000")-1];
}
yf_time_data_t;

/*
* add when 2012-08-23(7.7) for supporting thread multi evt driver...
*/
#if defined (YF_MULTI_EVT_DRIVER) && !defined ___YF_THREAD
yf_time_data_t* yf_time_data_addr();
#define yf_time_data yf_time_data_addr()
#else

extern ___YF_THREAD yf_time_data_t  yf_time_data_ins;
#define yf_time_data (&yf_time_data_ins)

#endif

#define yf_now_times (yf_time_data->now_times)
#define yf_log_time (yf_time_data->log_time)


typedef  void (*yf_time_reset_handler)(yf_utime_t* new_time, yf_time_t* diff_tm, void* data);

yf_int_t  yf_init_time(yf_log_t* log);
yf_int_t  yf_update_time(yf_time_reset_handler handle, void* data, yf_log_t* log);

void yf_localtime(time_t s, yf_stm_t *tm);

yf_int_t yf_real_walltime(yf_time_t* time);

#endif

