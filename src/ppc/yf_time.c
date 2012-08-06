#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>


yf_times_t  yf_now_times;
yf_times_t  yf_start_times;
yf_str_t      yf_log_time;

static void yf_update_log_time(yf_log_t* log);

#define yf_utime_to_time(times, utime) do { \
        times.tv_sec = utime.tv_sec; \
        times.tv_msec = utime.tv_usec / 1000; \
} while (0)


#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)

static yf_utime_t  yf_last_clock_utime;
static yf_utime_t  yf_last_real_wall_utime;

#define  YF_WALL_GET_INTERVAL_TMS  120

yf_int_t  yf_init_time(yf_log_t* log)
{
        if (gettimeofday(&yf_start_times.wall_utime, NULL) != 0)
        {
                yf_log_error(YF_LOG_WARN, log, yf_errno, "gettimeofday err");
                return YF_ERROR;
        }

        yf_last_real_wall_utime = yf_start_times.wall_utime;

        //clock time
        struct timespec ts;
        
        if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        {
                yf_log_error(YF_LOG_WARN, log, yf_errno, "gettime err");
                return YF_ERROR;
        }
        
        yf_last_clock_utime.tv_sec = ts.tv_sec;
        yf_last_clock_utime.tv_usec = ts.tv_nsec / 1000;

        yf_start_times.clock_utime = yf_last_clock_utime;

        yf_utime_to_time(yf_start_times.clock_time, yf_start_times.clock_utime);

        yf_log_time.len = sizeof("70/01/01 12:00:00 000") - 1;
        yf_log_time.data = yf_alloc(yf_log_time.len);

        yf_update_log_time(log);

        return  YF_OK;
}


yf_int_t  yf_update_time(yf_time_reset_handler handle, void* data, yf_log_t* log)
{
        struct timespec ts;
        
        if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        {
                yf_log_error(YF_LOG_WARN, log, yf_errno, "gettime err");
                return YF_ERROR;
        }
        
        yf_now_times.clock_utime.tv_sec = ts.tv_sec;
        yf_now_times.clock_utime.tv_usec = ts.tv_nsec / 1000;

        yf_utime_to_time(yf_now_times.clock_time, yf_now_times.clock_utime);

        if (yf_now_times.clock_utime.tv_sec < yf_last_clock_utime.tv_sec
                + YF_WALL_GET_INTERVAL_TMS)
        {
                yf_now_times.wall_utime.tv_usec = yf_last_real_wall_utime.tv_usec 
                                + yf_now_times.clock_utime.tv_usec 
                                - yf_last_clock_utime.tv_usec;
                
                yf_now_times.wall_utime.tv_sec = yf_last_real_wall_utime.tv_sec
                                + yf_now_times.clock_utime.tv_sec 
                                - yf_last_clock_utime.tv_sec;
                
                if ((yf_s32_t)yf_now_times.wall_utime.tv_usec < 0)
                {
                        yf_now_times.wall_utime.tv_usec += 1000000;
                        --yf_now_times.wall_utime.tv_sec;
                }
                else if (yf_now_times.wall_utime.tv_usec >= 1000000) {
                        yf_now_times.wall_utime.tv_usec -= 1000000;
                        ++yf_now_times.wall_utime.tv_sec;
                }

                yf_update_log_time(log);
                return  YF_OK;
        }

        yf_log_debug0(YF_LOG_DEBUG, log, 0, "update real wall time now");

        yf_last_clock_utime = yf_now_times.clock_utime;

        if (gettimeofday(&yf_now_times.wall_utime, NULL) != 0)
        {
                yf_log_error(YF_LOG_WARN, log, yf_errno, "gettimeofday err");
                return YF_ERROR;
        }

        yf_last_real_wall_utime = yf_now_times.wall_utime;
        yf_update_log_time(log);
        return  YF_OK;
}

#else

#ifdef __GNUC__
#warning "cant find clock_gettime function, use gettimeofday"
#else
#pragma message("cant find clock_gettime function, use gettimeofday")
#endif

yf_int_t  yf_init_time(yf_log_t* log)
{
        if (gettimeofday(&yf_start_times.wall_utime, NULL) != 0)
        {
                yf_log_error(YF_LOG_WARN, log, yf_errno, "gettimeofday err");
                return YF_ERROR;
        }

        yf_start_times.clock_utime = yf_start_times.wall_utime;
        yf_utime_to_time(yf_start_times.clock_time, yf_start_times.clock_utime);

        yf_update_log_time();
        return  YF_OK;
}

yf_int_t  yf_update_time(yf_time_reset_handler handle, void* data, yf_log_t* log)
{
        yf_utime_t  now_time;
        
        if (gettimeofday(&now_time, NULL) != 0)
        {
                yf_log_error(YF_LOG_WARN, log, yf_errno, "gettimeofday err");
                return YF_ERROR;
        }
        
        if (yf_utime_cmp(&now_time, &yf_now_times.wall_utime, >=))
        {
                yf_now_times.wall_utime = yf_now_times.clock_utime = now_time;
                yf_utime_to_time(yf_now_times.clock_time, yf_now_times.clock_utime);

                yf_update_log_time();
                return  YF_OK;
        }

        //correct time
        yf_time_t  diff_time;
        diff_time.tv_sec = yf_now_times.wall_utime.tv_sec - now_time.tv_sec;
        diff_time.tv_msec = (yf_now_times.wall_utime.tv_usec - now_time.tv_usec) / 1000;

        if (handle)
                handle(&now_time, &diff_time, data);
        
        yf_now_times.wall_utime = yf_now_times.clock_utime = now_time;
        yf_utime_to_time(yf_now_times.clock_time, yf_now_times.clock_utime);

        yf_start_times.wall_utime = yf_start_times.clock_utime = now_time;
        yf_start_times.clock_time = yf_now_times.clock_time;

        yf_update_log_time();
        return  YF_OK;
}

#endif


void yf_localtime(time_t s, yf_stm_t *tm)
{
#if (HAVE_LOCALTIME_R)
        (void) localtime_r(&s, tm);
#else
        yf_stm_t  *t;

        t = localtime(&s);
        *tm = *t;
#endif

        tm->yf_tm_mon++;
        tm->yf_tm_year += 1900;        
}


static void yf_update_log_time(yf_log_t* log)
{
        yf_stm_t  tm;
        yf_localtime(yf_now_times.wall_utime.tv_sec, &tm);

        yf_snprintf(yf_log_time.data, yf_log_time.len, 
                "%2d/%02d/%02d %02d:%02d:%02d %03d",
                tm.yf_tm_year - 2000, tm.yf_tm_mon,
                tm.yf_tm_mday, tm.yf_tm_hour,
                tm.yf_tm_min, tm.yf_tm_sec, 
                yf_now_times.wall_utime.tv_usec >> 10);
}
