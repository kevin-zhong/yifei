#ifndef _YF_LOG_FILE_H_20130209_H
#define _YF_LOG_FILE_H_20130209_H
/*
* copyright@: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20130209-13:39 ^_^ chunjie ^_^
*/

#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <bridge/yf_bridge.h>


typedef struct
{
        yf_uint_t  log_cache_size;
        yf_uint_t  switch_log_size;
        yf_uint_t  recur_log_num;
        const char* log_file_path;

        //if NULL, use default pre format
        const char* log_pre_format;
}
yf_log_file_init_ctx_t;

yf_int_t yf_log_file_init(yf_log_t* log);

yf_int_t yf_log_file_flush_drive(yf_evt_driver_t* evt_driver
                , yf_uint_t flush_ms, yf_log_t* log);


typedef char* (*yf_log_file_handle_t)(char* buf, char* last, yf_uint_t level);

yf_int_t yf_log_file_add_handle(yf_log_t* log, char symbol, yf_log_file_handle_t h);


#endif
