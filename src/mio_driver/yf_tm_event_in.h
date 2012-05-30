#ifndef  _YF_TM_EVENT_IN_H
#define _YF_TM_EVENT_IN_H

#include "yf_event_base_in.h"

struct  yf_timer_s
{
        union {
                yf_list_part_t   linker;
                yf_slist_part_t  free_linker;
        };
        yf_list_part_t  *head;

        //timeout val
        yf_time_t  time_out;
        yf_time_t  time_start;
        yf_time_t  time_org_start;
};


struct  yf_tm_evt_link_s
{
        yf_timer_t     timer;
        yf_tm_evt_t   evt;
};


#define  yf_link_2_timer(pos) container_of(pos, yf_timer_t, linker)

#define  yf_set_timer_val(event, out_val, is_fd_evt) do { \
        event->timer.time_out = out_val; \
        event->timer.time_start = yf_now_times.clock_time; \
        event->timer.time_org_start = yf_now_times.clock_time; \
        if (is_fd_evt) \
                event->timer.time_org_start.tv_msec |= 1; \
        else \
                event->timer.time_org_start.tv_msec = ((event->timer.time_org_start.tv_msec|1) - 1); \
} while (0)

#define  yf_is_fd_evt(timer) ((timer)->time_org_start.tv_msec & 1)


typedef  union
{
        yf_u64_t  bit_64;
        union {
                yf_u32_t  bit_32;
                yf_u16_t  bit_16[2];
        } ubits[2];
}
yf_bit_set_t;


#define  YF_TIMER_PRCS_MS_BIT   2
#define  YF_FAR_TIMER_ROLL_SIZE_BIT 10

//1024
#define  _YF_FAR_TIMER_ROLL_SIZE  (1<<YF_FAR_TIMER_ROLL_SIZE_BIT)
// 4
#define  _YF_TIMER_PRCS_MS  (1<<YF_TIMER_PRCS_MS_BIT)
// 8
#define  _YF_FAR_TIMER_PRCS_MS_BIT  (YF_TIMER_PRCS_MS_BIT + 7 - 1)

// 256
#define  _YF_FAR_TIMER_PRCS_MS   (1<<_YF_FAR_TIMER_PRCS_MS_BIT)
// 512
#define  _YF_NEAR_TIMER_MS_DIST (_YF_FAR_TIMER_PRCS_MS<<1)
#define  _YF_FAR_TIMER_MS_DIST (_YF_FAR_TIMER_PRCS_MS<<YF_FAR_TIMER_ROLL_SIZE_BIT)


typedef  struct
{
        yf_u32_t            stm_evts_capcity;

        yf_tm_evt_link_t  *pevents;
        yf_log_t               *log;

        yf_u32_t            stm_evts_num  ____cacheline_aligned;
        yf_u32_t            fdtm_evts_num;

        yf_u64_t               far_tm_period_cnt;
        yf_u64_t               too_far_tm_period_cnt;        

        yf_slist_part_t       free_list;

        yf_list_part_t        too_far_tm_list;
        
        yf_u32_t               far_tm_roll_index;
        yf_list_part_t        far_tm_lists[_YF_FAR_TIMER_ROLL_SIZE];
        
        yf_list_part_t        near_tm_lists[128]  ____cacheline_aligned;
        yf_bit_set_t          near_tm_flags[2];
        yf_u32_t               near_evts_num;
        yf_u32_t               near_tm_roll_index;

        yf_u64_t               tm_period_cnt;
        
        yf_u32_t               nearest_timeout_ms;
        
}   ____cacheline_aligned 
yf_tm_evt_driver_in_t;


#ifdef  WORDS_BIGENDIAN
#define  WORDS_LITTLE_PART  1
#else
#define  WORDS_LITTLE_PART  0
#endif

#define  YF_IS_LT_PART(p) (p == WORDS_LITTLE_PART)

#define  YF_SET_FLAG(flags, i) yf_set_bit(flags[i >> 6].bit_64, yf_mod(i, 64))
#define  YF_RSET_FLAG(flags, i) yf_reset_bit(flags[i >> 6].bit_64,  yf_mod(i, 64))


yf_int_t   yf_init_tm_driver(yf_tm_evt_driver_in_t* tm_driver
                , yf_u32_t nstimers, yf_log_t* log);

#define  yf_destory_tm_driver(tm_driver) yf_free((tm_driver)->pevents)

yf_int_t   yf_add_timer(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_timer_t* timer, yf_log_t *log);

yf_int_t   yf_del_timer(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_timer_t* timer, yf_log_t *log);

yf_int_t   yf_poll_timer(yf_tm_evt_driver_in_t* tm_evt_driver);

void  yf_on_time_reset(yf_utime_t* new_time, yf_time_t* diff_tm, void* data);


#endif

