#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include "yf_event_base_in.h"

static void yf_update_nearest_tm_ms(yf_tm_evt_driver_in_t* tm_evt_driver);

static void yf_add_near_timer(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_timer_t* timer, yf_s32_t ms, yf_log_t* log);
static void yf_list_timeout_handler(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_list_part_t *list, yf_int_t is_nearest);
static void yf_timeout_handler(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_timer_t* ptimer, yf_int_t is_nearest);
static yf_int_t yf_check_near_timer_list(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_u8_t list_offset, yf_int_t diff_periods, yf_u16_t bit_val);

static yf_int_t yf_poll_timer(yf_tm_evt_driver_in_t* tm_evt_driver);

static void  yf_timeout_caller(yf_tm_evt_driver_in_t* tm_evt_driver);


#define yf_far_timer_inc(tm_evt_driver, i) yf_cicur_add( \
                        tm_evt_driver->far_tm_roll_index, \
                        i, _YF_FAR_TIMER_ROLL_SIZE)

#define yf_near_timer_inc(tm_evt_driver, i) yf_cicur_add( \
                        tm_evt_driver->near_tm_roll_index, i, 128)


yf_int_t   yf_init_tm_driver(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_u32_t nstimers, yf_log_t* log)
{
        yf_s32_t i;
        
        tm_evt_driver->pevents = yf_alloc(sizeof(yf_tm_evt_link_t) * nstimers);
        if (tm_evt_driver->pevents == NULL) {
                yf_log_error(YF_LOG_ERR, log, yf_errno, "alloc tm pevents failed");
                return YF_ERROR;
        }

        yf_init_slist_head(&tm_evt_driver->timeout_list);
        yf_init_slist_head(&tm_evt_driver->free_list);

        for (i = (yf_s32_t)nstimers-1; i >= 0; --i)
        {
                yf_slist_push(&tm_evt_driver->pevents[i].timer.free_linker, 
                                &tm_evt_driver->free_list);
        }
        
        for (i = 0; (yf_u32_t)i < YF_ARRAY_SIZE(tm_evt_driver->far_tm_lists); ++i)
                yf_init_list_head(&tm_evt_driver->far_tm_lists[i]);

        yf_init_list_head(&tm_evt_driver->too_far_tm_list);

        for (i = 0; (yf_u32_t)i < YF_ARRAY_SIZE(tm_evt_driver->near_tm_lists); ++i)
                yf_init_list_head(&tm_evt_driver->near_tm_lists[i]);

        tm_evt_driver->tm_period_cnt = 0;
        tm_evt_driver->poll = yf_poll_timer;
        tm_evt_driver->log = log;

        //preset tm ms (there is no timers)
        yf_update_nearest_tm_ms(tm_evt_driver);

        return  YF_OK;
}


#define  LIST_ADD_TIMER(timer, list_head) do { \
        yf_list_add_tail(&(timer)->linker, list_head); \
        (timer)->head = list_head; \
} while (0)


static inline void yf_add_near_timer(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_timer_t* timer, yf_s32_t ms, yf_log_t* log)
{
        assert(ms >= _YF_TIMER_PRCS_MS && ms <= _YF_NEAR_TIMER_MS_DIST);
        
        yf_u32_t  index = (ms >> YF_TIMER_PRCS_MS_BIT) - 1;
        YF_SET_FLAG(tm_evt_driver->near_tm_flags, index);

        index = yf_near_timer_inc(tm_evt_driver, index);
        
        LIST_ADD_TIMER(timer, &tm_evt_driver->near_tm_lists[index]);

        tm_evt_driver->near_evts_num++;

        yf_log_debug5(YF_LOG_DEBUG, log, 0, "near timer roll index=%d, "
                        "add timer index=%d, tm ms=%d, near evts num=%d, timer addr=%p", 
                        tm_evt_driver->near_tm_roll_index, index, ms,  
                        tm_evt_driver->near_evts_num, 
                        timer);
}

yf_int_t   yf_add_timer(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_timer_t* timer, yf_log_t *log)
{
        if (yf_list_linked(&timer->linker))
        {
                yf_list_del(&timer->linker);
        }
        else {
                if (yf_is_fd_evt(timer))
                        tm_evt_driver->fdtm_evts_num++;
                else
                        tm_evt_driver->stm_evts_num++;

                yf_log_debug3(YF_LOG_DEBUG, log, 0, "fd tm num=%d, single tm num=%d, addr=%p", 
                                tm_evt_driver->fdtm_evts_num, 
                                tm_evt_driver->stm_evts_num, 
                                timer);
        }

        yf_s32_t  ms = yf_max(_YF_TIMER_PRCS_MS, yf_time_2_ms(&timer->time_out));
        ms = yf_align(ms, _YF_TIMER_PRCS_MS);
        
        yf_s32_t  diff_ms = 0, pass_mod_ms = 0;
        yf_u32_t  index = 0;
        
        if (ms >= _YF_FAR_TIMER_MS_DIST)
        {
                yf_log_debug1(YF_LOG_DEBUG, log, 0, "add too far tm, ms=%d", ms);
                
                yf_list_add_tail(&timer->linker, &tm_evt_driver->too_far_tm_list);
                return  YF_OK;
        }

        if (ms >= _YF_NEAR_TIMER_MS_DIST)
        {
                pass_mod_ms = yf_mod((tm_evt_driver->tm_period_cnt << YF_TIMER_PRCS_MS_BIT), 
                                        _YF_FAR_TIMER_PRCS_MS);
                
                diff_ms = ms - (_YF_FAR_TIMER_PRCS_MS - pass_mod_ms);
                
                index = yf_far_timer_inc(tm_evt_driver, (diff_ms >> _YF_FAR_TIMER_PRCS_MS_BIT));
                
                yf_log_debug4(YF_LOG_DEBUG, log, 0, "far timer roll index=%d, add timer index=%d, "
                                "pass_mod_ms=%d, diff_ms=%d", 
                                tm_evt_driver->far_tm_roll_index, index, 
                                pass_mod_ms, diff_ms);
                
                LIST_ADD_TIMER(timer, &tm_evt_driver->far_tm_lists[index]);
                return  YF_OK;
        }
        
        yf_add_near_timer(tm_evt_driver, timer, ms, log);
        return  YF_OK;
}


yf_int_t   yf_del_timer(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_timer_t* timer, yf_log_t *log)
{
        yf_u32_t  index = 0;
        
        if (!yf_list_linked(&timer->linker))
                return  YF_ERROR;

        if (yf_is_fd_evt(timer)) {
                tm_evt_driver->fdtm_evts_num--;
                assert(tm_evt_driver->fdtm_evts_num >= 0);
        }
        else {
                tm_evt_driver->stm_evts_num--;
                assert(tm_evt_driver->stm_evts_num >= 0);
        }

        yf_log_debug2(YF_LOG_DEBUG, log, 0, "fd tm num=%d, single tm num=%d", 
                        tm_evt_driver->fdtm_evts_num, 
                        tm_evt_driver->stm_evts_num);

        yf_list_del(&timer->linker);

        if (timer->head >= tm_evt_driver->near_tm_lists 
                && timer->head < tm_evt_driver->near_tm_lists + 128)
        {
                if (yf_list_empty(timer->head))
                {
                        index = yf_mod(timer->head - tm_evt_driver->near_tm_lists + 128 
                                - tm_evt_driver->near_tm_roll_index, 128);

                        yf_log_debug1(YF_LOG_DEBUG, log, 0, "near timer list_%d empty again", 
                                        index);

                        YF_RSET_FLAG(tm_evt_driver->near_tm_flags, index);
                }
                tm_evt_driver->near_evts_num--;

                yf_log_debug1(YF_LOG_DEBUG, log, 0, "near timer evts num=%d", 
                                tm_evt_driver->near_evts_num);
        }
        return  YF_OK;
}


static void yf_timeout_handler(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_timer_t* ptimer, yf_int_t is_nearest)
{
        //note, must -- first, last call biz call back func...
        if (is_nearest)
                tm_evt_driver->near_evts_num--;
        ptimer->head = NULL;

        yf_slist_push(&ptimer->free_linker, &tm_evt_driver->timeout_list);
}


static void  yf_timeout_caller(yf_tm_evt_driver_in_t* tm_evt_driver)
{
        yf_slist_part_t* pos;
        yf_timer_t* ptimer;

        //TODO, still have exception, if callback cancel a timeout timer...
        while (!yf_slist_empty(&tm_evt_driver->timeout_list))
        {
                pos = yf_slist_pop(&tm_evt_driver->timeout_list);
                ptimer = container_of(pos, yf_timer_t, free_linker);

                if (yf_is_fd_evt(ptimer))
                {
                        yf_fd_evt_link_t* fd_evt = container_of(ptimer, yf_fd_evt_link_t, timer);

                        tm_evt_driver->fdtm_evts_num--;

                        yf_log_debug3(YF_LOG_DEBUG, fd_evt->evt.log, 0, 
                                        "fd=%d[%V] timer out, fdtm evt num=%d", 
                                        fd_evt->evt.fd, &yf_evt_tn(&fd_evt->evt), 
                                        tm_evt_driver->fdtm_evts_num
                                        );

                        fd_evt->evt.timeout = 1;
                        fd_evt->evt.fd_evt_handler(&fd_evt->evt);
                }
                else {
                        yf_tm_evt_link_t* tm_evt = container_of(ptimer, yf_tm_evt_link_t, timer);

                        tm_evt_driver->stm_evts_num--;

                        yf_log_debug3(YF_LOG_DEBUG, tm_evt->evt.log, 0, 
                                        "single timer out, stm evt num=%d, near evt num=%d, addr=%p", 
                                        tm_evt_driver->stm_evts_num, 
                                        tm_evt_driver->near_evts_num, 
                                        &tm_evt->timer
                                        );
                        
                        tm_evt->evt.timeout_handler(&tm_evt->evt, &tm_evt->timer.time_org_start);
                }        
        }
}


static void yf_list_timeout_handler(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_list_part_t *list, yf_int_t is_nearest)
{
        yf_list_part_t *pos, *keep;
        yf_timer_t* ptimer;

        yf_list_for_each_safe(pos, keep, list)
        {
                ptimer = yf_link_2_timer(pos);

                //note, must delete link first, sometimes, timeout handle will call tm calls
                yf_list_del(pos);
                yf_timeout_handler(tm_evt_driver, ptimer, is_nearest);
        }
}


static yf_int_t yf_check_near_timer_list(yf_tm_evt_driver_in_t* tm_evt_driver
                , yf_u8_t list_offset, yf_int_t diff_periods, yf_u16_t bit_val)
{
        yf_u8_t  index_begin = yf_mod(tm_evt_driver->near_tm_roll_index 
                        + list_offset, 128);
        yf_s8_t*  unempty_indexs = yf_bit_indexs[bit_val].indexs;
        yf_u8_t   index_it = 0;
        yf_list_part_t *list = NULL;
        int i;

        for (i = 0; !yf_index_end(unempty_indexs, i) && unempty_indexs[i] < diff_periods; ++i)
        {
                index_it = yf_mod(index_begin + unempty_indexs[i], 128);
                list = tm_evt_driver->near_tm_lists + index_it;

                yf_log_debug2(YF_LOG_DEBUG, tm_evt_driver->log, 0, 
                                "near tm roll index=%d, check index_it=%d", 
                                tm_evt_driver->near_tm_roll_index, 
                                index_it
                                );

                assert(!yf_list_empty(list));

                yf_list_timeout_handler(tm_evt_driver, list, 1);
        }

        return YF_OK;
}


static void yf_update_nearest_tm_ms(yf_tm_evt_driver_in_t* tm_evt_driver)
{
#define _YF_SET_TM_MS(bitval, offset_bit) \
        tm_evt_driver->nearest_timeout_ms \
                = ((yf_bit_indexs[bitval].indexs[0] + offset_bit + 1) << YF_TIMER_PRCS_MS_BIT); \
        yf_log_debug4(YF_LOG_DEBUG, tm_evt_driver->log, 0, \
                "first_64=%L, second_64=%L, nearest tm index=%d, nearest tm=%ud", \
                bit_set[0].bit_64, bit_set[1].bit_64, \
                yf_bit_indexs[bitval].indexs[0] + offset_bit, tm_evt_driver->nearest_timeout_ms);

#define  _YF_GET_NEARSET_TM_MS(bits32, offset_bit) \
        if (bits32[WORDS_LITTLE_PART].bit_32) \
        { \
                if (bits32[WORDS_LITTLE_PART].bit_16[WORDS_LITTLE_PART]) { \
                        _YF_SET_TM_MS(bits32[WORDS_LITTLE_PART].bit_16[WORDS_LITTLE_PART], offset_bit); \
                } \
                else {\
                        _YF_SET_TM_MS(bits32[WORDS_LITTLE_PART].bit_16[1-WORDS_LITTLE_PART], offset_bit+16); \
                } \
        } \
        else { \
                if (bits32[1-WORDS_LITTLE_PART].bit_16[WORDS_LITTLE_PART]) { \
                        _YF_SET_TM_MS(bits32[1-WORDS_LITTLE_PART].bit_16[WORDS_LITTLE_PART], offset_bit+32); \
                } \
                else { \
                        _YF_SET_TM_MS(bits32[1-WORDS_LITTLE_PART].bit_16[1-WORDS_LITTLE_PART], offset_bit+48); \
                } \
        }

        yf_bit_set_t* bit_set = tm_evt_driver->near_tm_flags;
        yf_list_part_t *list;
        
        if (tm_evt_driver->near_evts_num)
        {
                if (bit_set[0].bit_64)
                {
                        _YF_GET_NEARSET_TM_MS(bit_set[0].ubits, 0);
                }
                else {
                        assert(bit_set[1].bit_64);
                        _YF_GET_NEARSET_TM_MS(bit_set[1].ubits, 64);
                }
        }
        else {
                yf_int_t unempty_inedex = 1;
                
                for (; unempty_inedex < 3; ++unempty_inedex)
                {
                        list = tm_evt_driver->far_tm_lists 
                                        + yf_far_timer_inc(tm_evt_driver, unempty_inedex);
                        if (!yf_list_empty(list))
                                break;
                }

                //max check time=3*_YF_FAR_TIMER_PRCS_MS=0.768s
                tm_evt_driver->nearest_timeout_ms 
                                = unempty_inedex << _YF_FAR_TIMER_PRCS_MS_BIT;

                yf_log_debug2(YF_LOG_DEBUG, tm_evt_driver->log, 0, 
                                "no near evts, unempty_index=%d so nearest_timeout_ms=%d", 
                                unempty_inedex, 
                                tm_evt_driver->nearest_timeout_ms);
        }       
}


yf_int_t   yf_poll_timer(yf_tm_evt_driver_in_t* tm_evt_driver)
{
        yf_u64_t  passed_periods = (yf_time_diff_ms(&yf_now_times.clock_time, 
                        &yf_start_times.clock_time) >> YF_TIMER_PRCS_MS_BIT);

        /*
        * check near timer
        */
        yf_s64_t  diff_periods = passed_periods - tm_evt_driver->tm_period_cnt;
        
        assert(diff_periods >= 0);
        if (diff_periods == 0)
                return  YF_OK;

        tm_evt_driver->tm_period_cnt = passed_periods;

        yf_log_debug3(YF_LOG_DEBUG, tm_evt_driver->log, 0, 
                        "prev near tm index=%d, poll timer after periods=%L, tm period cnt=%d", 
                        tm_evt_driver->near_tm_roll_index, 
                        diff_periods, tm_evt_driver->tm_period_cnt);

        yf_int_t  tmp_diff_periods = yf_min(diff_periods, 128);
        yf_bit_set_t* bit_set = NULL;

        yf_int_t  flag_index = 0;
        yf_int_t  words_part = 0;
        yf_u16_t*  bit_val = NULL;
        int i, check_index;

        if (tm_evt_driver->near_evts_num)
        {
check_near_timer:
                bit_set = tm_evt_driver->near_tm_flags + flag_index;

                if (bit_set->bit_64)
                {
                        words_part = WORDS_LITTLE_PART;

                        for (i = 0; i < 2; ++i)
                        {
                                yf_log_debug3(YF_LOG_DEBUG, tm_evt_driver->log, 0, 
                                                "[%d][%d]=%ud", flag_index, words_part, 
                                                bit_set->ubits[words_part].bit_32);
                                
                                if (bit_set->ubits[words_part].bit_32)
                                {
#define  YF_TEST_BIT16(_part) \
                                        bit_val = &bit_set->ubits[words_part].bit_16[_part]; \
                                        if (*bit_val) { \
                                                yf_check_near_timer_list(tm_evt_driver  \
                                                        , (flag_index ? 64 : 0) \
                                                        + (YF_IS_LT_PART(words_part) ? 0 : 32) \
                                                        + (YF_IS_LT_PART(_part) ? 0 : 16) \
                                                        , tmp_diff_periods, *bit_val); \
                                        } \
                                        tmp_diff_periods -= 16;

                                        YF_TEST_BIT16(WORDS_LITTLE_PART);
                                        
                                        if (tmp_diff_periods <= 0)
                                                goto  update_near_flags;
                                        
                                        YF_TEST_BIT16(1 - WORDS_LITTLE_PART);
                                }
                                else
                                        tmp_diff_periods -= 32;

                                if (tmp_diff_periods <= 0)
                                        goto  update_near_flags;

                                words_part = 1 - words_part;
                        }
                }
                else
                        tmp_diff_periods -= 64;

                if (tmp_diff_periods > 0) 
                {
                        flag_index = 1;
                        goto  check_near_timer;
                }

update_near_flags:
                bit_set = tm_evt_driver->near_tm_flags;
                if (diff_periods < 128 && tm_evt_driver->near_evts_num)
                {
                        tm_evt_driver->near_tm_roll_index = yf_mod(
                                tm_evt_driver->near_tm_roll_index + diff_periods, 128);

                        yf_log_debug2(YF_LOG_DEBUG, tm_evt_driver->log, 0, 
                                        "now tm roll index=%d, near evts num=%d, ", 
                                        tm_evt_driver->near_tm_roll_index, 
                                        tm_evt_driver->near_evts_num);

                        if (diff_periods < 64)
                        {
                                bit_set[0].bit_64 = ((bit_set[0].bit_64 >> diff_periods) 
                                                | (bit_set[1].bit_64 << (64 - diff_periods)));
                                bit_set[1].bit_64 >>= diff_periods;
                        }
                        else {
                                bit_set[0].bit_64 = (bit_set[1].bit_64 >> (diff_periods - 64));
                                bit_set[1].bit_64 = 0;
                        }
                }
                else {
                        if (diff_periods >= 128)
                                assert(tm_evt_driver->near_evts_num == 0);

                        yf_log_debug0(YF_LOG_DEBUG, tm_evt_driver->log, 0, 
                                        "near evts num all=0");              

                        yf_memzero_st(tm_evt_driver->near_tm_flags);
                }
        }

        /*
        * check far timer
        */
        passed_periods >>= (_YF_FAR_TIMER_PRCS_MS_BIT - YF_TIMER_PRCS_MS_BIT);
        diff_periods = passed_periods - tm_evt_driver->far_tm_period_cnt;
        assert(diff_periods >= 0);
        if (diff_periods == 0)
        {
                //will call this each near checker !!
                yf_update_nearest_tm_ms(tm_evt_driver);
                yf_timeout_caller(tm_evt_driver);
                return YF_OK;
        }

        tm_evt_driver->far_tm_period_cnt = passed_periods;

        yf_log_debug3(YF_LOG_DEBUG, tm_evt_driver->log, 0, 
                        "prev far tm index=%d, poll far timer after periods=%L, "
                        "far_tm_period_cnt=%d", 
                        tm_evt_driver->far_tm_roll_index, 
                        diff_periods, tm_evt_driver->far_tm_period_cnt);

        yf_list_part_t *list, *pos, *keep;
        yf_timer_t *timer;
        yf_u32_t  time_ms;
        yf_s64_t  passed_tv;

#define  MV_FROM_FAR_TO_NEAR(_timer, _rest_ms) \
                _timer->time_out.tv_msec = yf_align(_rest_ms, _YF_TIMER_PRCS_MS); \
                _timer->time_out.tv_sec = 0; \
                _timer->time_start = yf_now_times.clock_time; \
                yf_add_near_timer(tm_evt_driver, _timer, _timer->time_out.tv_msec, \
                                tm_evt_driver->log);

        //timeout far list
        if (diff_periods > 1)
        {
                //cause, prev 1 lists are empty
                for (i = 1; i < diff_periods - 1; ++i)
                {
                       check_index = yf_far_timer_inc(tm_evt_driver, i);
                       
                       yf_log_debug2(YF_LOG_DEBUG, tm_evt_driver->log, 0, 
                                        "far tm roll index=%d, check index=%ud", 
                                        tm_evt_driver->far_tm_roll_index, check_index
                                        );
                        
                        yf_list_timeout_handler(tm_evt_driver, 
                                        tm_evt_driver->far_tm_lists + check_index, 0);
                }
                
                //mid list(may contain timeout timer or nearest timer)
                check_index = yf_far_timer_inc(tm_evt_driver, diff_periods - 1);
                list = tm_evt_driver->far_tm_lists + check_index;
                
                yf_list_for_each_safe(pos, keep, list)
                {
                        timer = yf_link_2_timer(pos);
                        
                        time_ms = yf_time_2_ms(&timer->time_out);
                        passed_tv = yf_time_diff_ms(&yf_now_times.clock_time, 
                                        &timer->time_start);

                        yf_list_del(pos);
                        
                        if (passed_tv >= time_ms)
                        {
                                yf_log_debug3(YF_LOG_DEBUG, tm_evt_driver->log, 0, 
                                                "mid far tm timeout, check index=%d, passed_tv=%L, time_ms=%ud", 
                                                check_index, passed_tv, time_ms);
                                
                                yf_timeout_handler(tm_evt_driver, timer, 0);
                                continue;
                        }

                        yf_log_debug4(YF_LOG_DEBUG, tm_evt_driver->log, 0, 
                                        "mv from mid far list to near list, check index=%d, "
                                        "passed_tv=%L, time_ms=%ud, addr=%p", 
                                        check_index, passed_tv, time_ms, timer);

                        MV_FROM_FAR_TO_NEAR(timer, time_ms - passed_tv);                 
                }
        }

        //update far_tm_roll_index
        tm_evt_driver->far_tm_roll_index 
                        = yf_far_timer_inc(tm_evt_driver, diff_periods);

        yf_log_debug1(YF_LOG_DEBUG, tm_evt_driver->log, 0, 
                        "updated far_tm_roll_index=%d", 
                        tm_evt_driver->far_tm_roll_index);

        //move prev 1 far lists to near list
        check_index = tm_evt_driver->far_tm_roll_index;
        list = tm_evt_driver->far_tm_lists + check_index;
        
        yf_list_for_each_safe(pos, keep, list)
        {
                timer = yf_link_2_timer(pos);
                
                time_ms = yf_time_2_ms(&timer->time_out);
                passed_tv = yf_time_diff_ms(&yf_now_times.clock_time, 
                                &timer->time_start);
                
                yf_log_debug4(YF_LOG_DEBUG, tm_evt_driver->log, 0, 
                                "mv from far list to near list, check index=%d, "
                                "passed_tv=%L, time_ms=%ud, addr=%p", 
                                check_index, passed_tv, time_ms, timer);

                //below will core if in such case, tmp_period_cnt = 45311(org is 45234, 45311 will
                //arrive next far period in just 1 near period=4ms), and cause, tm align to 4, so org
                //maybe time_ms <= passed_tv
                //assert(passed_tv < time_ms);
                yf_list_del(pos);

                time_ms = yf_max(time_ms - passed_tv, _YF_TIMER_PRCS_MS);
                MV_FROM_FAR_TO_NEAR(timer, time_ms);
        }

        /*
        * check nearest timeout ms again...
        */
        yf_update_nearest_tm_ms(tm_evt_driver);

        /*
        * check too far timer(half far time dist)
        */
        passed_periods >>= (YF_FAR_TIMER_ROLL_SIZE_BIT - 1);
        diff_periods = passed_periods - tm_evt_driver->too_far_tm_period_cnt;
        assert(diff_periods >= 0);
        if (diff_periods == 0)
        {
                yf_timeout_caller(tm_evt_driver);
                return YF_OK;
        }

        tm_evt_driver->too_far_tm_period_cnt = passed_periods;

        yf_log_debug1(YF_LOG_DEBUG, tm_evt_driver->log, 0, 
                        "updated too_far_tm_period_cnt=%d", 
                        tm_evt_driver->too_far_tm_period_cnt);

        yf_u64_t  passed_ms, timeout_ms, rest_ms;
        yf_u32_t  index = 0;
        yf_int_t   big_lacy = 0;

        yf_list_for_each_safe(pos, keep, &tm_evt_driver->too_far_tm_list)
        {
                timer = yf_link_2_timer(pos);
                
                passed_ms = yf_time_diff_ms(&yf_now_times.clock_time, &timer->time_start);
                timeout_ms = yf_time_2_ms(&timer->time_out);
                if (passed_ms >= timeout_ms)//big lacy
                {
                        yf_log_error(YF_LOG_WARN, tm_evt_driver->log, 0, 
                                        "big lacy, timeout ts=%uL, passed ms=%uL", 
                                        timeout_ms, passed_ms);
                        rest_ms = 0;
                }
                else
                        rest_ms = timeout_ms - passed_ms;

                if (rest_ms >= _YF_FAR_TIMER_MS_DIST)
                        continue;
                
                yf_log_debug3(YF_LOG_DEBUG, tm_evt_driver->log, 0, "mv from too far list to pre list, "
                                "passed_tv=%d, time_ms=%d, addr=%p", 
                                passed_ms, timeout_ms, 
                                timer);
                
                timer->time_out.tv_msec = yf_align(rest_ms, _YF_TIMER_PRCS_MS);
                timer->time_out.tv_sec = 0;
                timer->time_start = yf_now_times.clock_time;

                if (rest_ms < _YF_NEAR_TIMER_MS_DIST)
                {
                        big_lacy = 1;
                }

                yf_add_timer(tm_evt_driver, timer, tm_evt_driver->log);
        }

        if (big_lacy)
                yf_update_nearest_tm_ms(tm_evt_driver);

        yf_timeout_caller(tm_evt_driver);
        return YF_OK;
}


void yf_on_time_reset(yf_utime_t* new_time
                , yf_time_t* diff_tm, void* data)
{
        yf_tm_evt_driver_in_t* tm_evt_driver = data;

        tm_evt_driver->tm_period_cnt = 0;
        tm_evt_driver->far_tm_period_cnt = 0;
        tm_evt_driver->too_far_tm_period_cnt = 0;
}


yf_int_t  yf_alloc_tm_evt(yf_evt_driver_t* driver, yf_tm_evt_t** tm_evt
                , yf_log_t* log)
{
        yf_tm_evt_driver_in_t* tm_evt_driver 
                        = &((yf_evt_driver_in_t*)driver)->tm_driver;

        if (yf_slist_empty(&tm_evt_driver->free_list))
        {
                yf_log_error(YF_LOG_ERR, log, 0, "tm event used out..");
                return  YF_ERROR;
        }

        yf_slist_part_t *link_part = yf_slist_pop(&tm_evt_driver->free_list);

        yf_tm_evt_link_t* alloc_evt = container_of(link_part, yf_tm_evt_link_t, timer.free_linker);
        yf_memzero(alloc_evt, sizeof(yf_tm_evt_link_t));

        *tm_evt = &alloc_evt->evt;
        (*tm_evt)->log = log;
        (*tm_evt)->driver = driver;
        return  YF_OK;
}

yf_int_t  yf_free_tm_evt(yf_tm_evt_t* tm_evt)
{
        yf_evt_driver_t* driver = tm_evt->driver;
        
        yf_tm_evt_driver_in_t* tm_evt_driver 
                        = &((yf_evt_driver_in_t*)driver)->tm_driver;
        yf_tm_evt_link_t* alloc_evt = container_of(tm_evt, yf_tm_evt_link_t, evt);
        
        yf_del_timer(tm_evt_driver, &alloc_evt->timer, tm_evt->log);

        if (yf_is_fd_evt(&alloc_evt->timer))
        {
                yf_log_error(YF_LOG_ERR, tm_evt->log, 0, "fd timer event, cant be free..");
                return  YF_ERROR;
        }

        yf_slist_push(&alloc_evt->timer.free_linker, &tm_evt_driver->free_list);
        return  YF_OK;
}


yf_int_t   yf_register_tm_evt(yf_tm_evt_t* tm_evt
                , yf_time_t  *time_out)
{
        yf_evt_driver_t* driver = tm_evt->driver;
        
        yf_evt_driver_in_t* evt_driver = (yf_evt_driver_in_t*)driver;
        yf_tm_evt_link_t* tm_evt_link = container_of(tm_evt, yf_tm_evt_link_t, evt);
        yf_set_timer_val(tm_evt_link, *time_out, 0);
        
        return  yf_add_timer(&evt_driver->tm_driver, 
                        &tm_evt_link->timer, tm_evt->log);
}


yf_int_t   yf_unregister_tm_evt(yf_tm_evt_t* tm_evt)
{
        yf_evt_driver_t* driver = tm_evt->driver;
        
        yf_evt_driver_in_t* evt_driver = (yf_evt_driver_in_t*)driver;
        
        return  yf_del_timer(&evt_driver->tm_driver, 
                        &container_of(tm_evt, yf_tm_evt_link_t, evt)->timer, tm_evt->log);
}

