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

        yf_time_t  time_start;

        yf_u32_t   time_out_ms:30;
        yf_u32_t   is_fd_evt:1;
};


struct  yf_tm_evt_link_s
{
        yf_timer_t     timer;
        yf_tm_evt_t   evt;
};


#define  yf_link_2_timer(pos) container_of(pos, yf_timer_t, linker)

#define  yf_set_timer_val(event, _out_val, _is_fd_evt) do { \
        event->timer.time_out_ms = yf_time_2_ms(_out_val); \
        event->timer.time_start = yf_now_times.clock_time; \
        event->timer.is_fd_evt = _is_fd_evt; \
} while (0)

#define  yf_is_fd_evt(timer) ((timer)->is_fd_evt)

//just these 2 pre val can set according to your need
#define  YF_TIMER_PRCS_MS_BIT   1
#define  YF_FAR_TIMER_ROLL_SIZE_BIT 10

//1024
#define  _YF_FAR_TIMER_ROLL_SIZE  (1<<YF_FAR_TIMER_ROLL_SIZE_BIT)
// 2
#define  _YF_TIMER_PRCS_MS  (1<<YF_TIMER_PRCS_MS_BIT)
// 7
#define  _YF_FAR_TIMER_PRCS_MS_BIT  (YF_TIMER_PRCS_MS_BIT + 6)

// 128
#define  _YF_FAR_TIMER_PRCS_MS   (1<<_YF_FAR_TIMER_PRCS_MS_BIT)
// 256
#define  _YF_NEAR_TIMER_MS_DIST (_YF_FAR_TIMER_PRCS_MS<<1)
#define  _YF_FAR_TIMER_MS_DIST (_YF_FAR_TIMER_PRCS_MS<<YF_FAR_TIMER_ROLL_SIZE_BIT)

/*
#define  YF_TIMER_PRCS_MS_BIT   2
#define  YF_FAR_TIMER_ROLL_SIZE_BIT 10

//1024
#define  _YF_FAR_TIMER_ROLL_SIZE  (1<<YF_FAR_TIMER_ROLL_SIZE_BIT)
// 4
#define  _YF_TIMER_PRCS_MS  (1<<YF_TIMER_PRCS_MS_BIT)
// 8
#define  _YF_FAR_TIMER_PRCS_MS_BIT  (YF_TIMER_PRCS_MS_BIT)

// 256
#define  _YF_FAR_TIMER_PRCS_MS   (1<<_YF_FAR_TIMER_PRCS_MS_BIT)
// 512
#define  _YF_NEAR_TIMER_MS_DIST (_YF_FAR_TIMER_PRCS_MS<<1)
#define  _YF_FAR_TIMER_MS_DIST (_YF_FAR_TIMER_PRCS_MS<<YF_FAR_TIMER_ROLL_SIZE_BIT)
*/

#define _YF_NEAREST_TIMER_ROLL_SIZE 128


struct  yf_tm_evt_driver_in_s
{
        yf_u32_t            stm_evts_capcity;

        yf_tm_evt_link_t  *pevents;
        yf_log_t               *log;

        yf_u32_t            stm_evts_num  ____cacheline_aligned;
        yf_u32_t            fdtm_evts_num;

        yf_u64_t               far_tm_period_cnt;
        yf_u64_t               too_far_tm_period_cnt;        

        yf_slist_part_t       timeout_list;

        yf_slist_part_t       free_list;

        yf_list_part_t        too_far_tm_list;
        
        yf_u32_t               far_tm_roll_index;
        yf_list_part_t        far_tm_lists[_YF_FAR_TIMER_ROLL_SIZE];
        
        yf_list_part_t        near_tm_lists[_YF_NEAREST_TIMER_ROLL_SIZE]  ____cacheline_aligned;
        yf_bit_set_t          near_tm_flags[2];
        yf_u32_t               near_evts_num;
        yf_u32_t               near_tm_roll_index;

        yf_u64_t               tm_period_cnt;
        
        yf_u32_t               nearest_timeout_ms;

        yf_int_t   (*poll)(struct  yf_tm_evt_driver_in_s* tm_evt_driver);
        
}   ____cacheline_aligned ;

#define  YF_SET_FLAG(flags, i) yf_set_bit(flags[i >> 6].bit_64, yf_mod(i, 64))
#define  YF_RSET_FLAG(flags, i) yf_reset_bit(flags[i >> 6].bit_64,  yf_mod(i, 64))


yf_int_t   yf_init_tm_driver(yf_tm_evt_driver_in_t* tm_driver
                , yf_u32_t nstimers, yf_log_t* log);

#define  yf_destory_tm_driver(tm_driver) yf_free((tm_driver)->pevents)

yf_int_t   yf_add_timer(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_timer_t* timer, yf_log_t *log, yf_u32_t tm_ms);

yf_int_t   yf_del_timer(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_timer_t* timer, yf_log_t *log);

void  yf_on_time_reset(yf_utime_t* new_time, yf_time_t* diff_tm, void* data);


#endif

