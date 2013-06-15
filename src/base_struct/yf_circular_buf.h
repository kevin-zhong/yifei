#ifndef _YF_CIRCULAR_BUF_H_20130321_H
#define _YF_CIRCULAR_BUF_H_20130321_H
/*
* copyright@: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20130321-21:01:35
*/

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

//base 
#define YF_CBR  >=
#define YF_CBW >

#define yf_cb_diff(m, n, capacity, op) (m op n ? (m-n) : (m+capacity-n))
#define yf_cb_add(i, n, capacity) (i+(n < (capacity-i) ? n : (yf_int_t)n - capacity))
#define yf_cb_sub(i, n, capacity) (i-(n > i ? (yf_int_t)n - capacity : n))
#define yf_cb_increase(i, capacity) ((i) == (capacity)-1 ? 0 : (i)+1)
#define yf_cb_decrease(i, capacity) ((i) == 0 ? (capacity)-1 : (i)-1)

#define yf_cb_rest_mod(i, capacity) ({yf_s32_t __r = yf_mod(i, capacity); \
                __r==0 ? capacity : __r;})


#define  YF_CIRCULAR_BUF_MAX_NUM  256

/*
* max mem capacity = buf_size * YF_CIRCULAR_BUF_MAX_NUM
*/
typedef struct yf_circular_buf_s
{
        yf_s32_t  buf_size;//=pow(2,n)
        
        //for expand buf, so if head_index == cursor_index
        //then cursor_offset >= head_offset(read rest<=buf_size)
        yf_u8_t    buf_size_pow;
        yf_u8_t    shrink_cnt;
        yf_s16_t   buf_used;//<=YF_CIRCULAR_BUF_MAX_NUM

        yf_s16_t  head_index;
        //if no seek, then cursor_index==tail_index && woffset==eoffset always
        yf_s16_t  cursor_index;
        yf_s16_t  tail_index;
        
        yf_s32_t  head_offset;
        yf_s32_t  cursor_offset;
        yf_s32_t  tail_offset;
        
        char*  buf[YF_CIRCULAR_BUF_MAX_NUM];
        
        //used for biz, not for buf alloc
        yf_pool_t* mpool;
}
yf_circular_buf_t;


yf_int_t  yf_circular_buf_init(yf_circular_buf_t* cb, yf_s32_t buf_size, yf_log_t * log);
void  yf_circular_buf_destory(yf_circular_buf_t* cb);

#define yf_circular_buf_reset(cb) \
        (cb)->head_index = 0; (cb)->head_offset = 0; \
        (cb)->cursor_index = 0; (cb)->cursor_offset = 0; \
        (cb)->tail_index = 0; (cb)->tail_offset = 0;

/*
* shrink, to reset biz pool, and to shrink mem buf alloced too many
*/
void  yf_circular_buf_shrink(yf_circular_buf_t* cb, yf_log_t * log);

#define yf_circular_buf_bsize(cb, bytes) \
        (yf_align(bytes, cb->buf_size) >> cb->buf_size_pow)

#define __yf_circular_index_offset(ai, ao, bi, bo, cb, op) \
        ((((yf_s32_t)yf_cb_diff(ai, bi, cb->buf_used, op)) << (cb->buf_size_pow)) + ao - bo)

#define yf_circular_buf_rest_rsize(cb) \
        __yf_circular_index_offset(cb->tail_index, cb->tail_offset, \
                        cb->cursor_index, cb->cursor_offset, cb, YF_CBR)
        
#define yf_circular_buf_free_wsize(cb) \
        __yf_circular_index_offset(cb->head_index, 0, \
                        cb->cursor_index, cb->cursor_offset, cb, YF_CBW)

#define yf_circular_buf_tailed_wsize(cb) \
        __yf_circular_index_offset(cb->head_index, 0, \
                        cb->tail_index, cb->tail_offset, cb, YF_CBW)

/*
* if (o == **) o = cb->buf_size - o ? why ? on 2013/03/25 22:02 at home !!
* cause rest can==cb->buf_size, but yf_mod cant desc this situation
*/
#define __yf_circular_buf_advance_back(i, o, diff, cb) do { \
        o -= diff; \
        if (o < 0) \
        { \
                yf_s32_t _restb = -o; \
                o = cb->buf_size - yf_cb_rest_mod(_restb, cb->buf_size); \
                yf_s16_t _restbsize = yf_circular_buf_bsize(cb, _restb); \
                i = yf_cb_sub(i, _restbsize, cb->buf_used); \
        } \
} while(0)

#define __yf_circular_buf_advance_front(i, o, diff, cb) do { \
        yf_s32_t  _wbytes = cb->buf_size - o; \
        if (diff <= _wbytes) \
                o += diff; \
        else { \
                _wbytes = diff - _wbytes; \
                o = yf_cb_rest_mod(_wbytes, cb->buf_size); \
                yf_s16_t _wbsize = yf_circular_buf_bsize(cb, _wbytes); \
                i = yf_cb_add(i, _wbsize, cb->buf_used); \
        } \
} while(0)

#define yf_circular_buf_advance(i, o, diff, cb) do { \
        if (diff > 0) \
                __yf_circular_buf_advance_front(i, o, diff, cb); \
        else \
                __yf_circular_buf_advance_back(i, o, (-diff), cb); \
} while(0)

#define YF_READ_PEEK 1

/*
* ---------------------file-like interface---------------------
*/
//if offset=0, use now cursor as start, should offset>=0
//next 3 apis, ret YF_OK if success, else error
yf_int_t yf_cb_fhead_set(yf_circular_buf_t* cb, yf_s32_t offset);
yf_int_t yf_cb_ftruncate(yf_circular_buf_t* cb, yf_s32_t size);
yf_int_t yf_cb_fseek(yf_circular_buf_t* cb, yf_s32_t offset, yf_int_t fromwhere);

#define yf_cb_fsize(cb) \
        __yf_circular_index_offset((cb)->tail_index, (cb)->tail_offset, \
                        (cb)->head_index, (cb)->head_offset, (cb), YF_CBR)

#define yf_cb_ftell(cb) \
        __yf_circular_index_offset((cb)->cursor_index, (cb)->cursor_offset, \
                        (cb)->head_index, (cb)->head_offset, (cb), YF_CBR)

//ret readed or writed success num
//flags=0 or YF_READ_PEEK
yf_s32_t yf_cb_fread(yf_circular_buf_t* cb, yf_s32_t bytes, yf_int_t flags, char** rbuf);
yf_s32_t yf_cb_fwrite(yf_circular_buf_t* cb, char* buf, yf_s32_t bytes);

/*
* --------------space directly operation interface, use carefully-------
*/
//if *rbufs != NULL, then use input buf(you must be sure the input size is big enough)
//else use inner
//ret read space bytes num, from rindex-roffset
//flags=0 or YF_READ_PEEK
yf_s32_t  yf_cb_space_read(yf_circular_buf_t* cb, yf_s32_t bytes
                , char*** rbufs, yf_s32_t* roffset, yf_int_t flags);

/*
* cautions, if use write_alloc with seek write the same time
* write just from the place=windex-woffset, not the end !!
* cursor and tail all unchanged
*/
//ret new free_wsize
yf_s32_t  yf_cb_space_enlarge(yf_circular_buf_t* cb, yf_s32_t bytes);
//ret writed space size
yf_s32_t  yf_cb_space_write_alloc(yf_circular_buf_t* cb, yf_s32_t bytes
                , char*** rbufs, yf_s32_t* woffset);

void yf_cb_space_write_bytes(yf_circular_buf_t* cb, yf_s32_t wreal_len);

/*
* help macros
*/
#define _yf_cb_space_get_impl(cb, bto, ifrom, bsize) do { \
        yf_s16_t _bsize = bsize; \
        yf_s16_t bsize_tmp = yf_min(_bsize, cb->buf_used-(ifrom)); \
        if (bsize_tmp) \
                yf_memcpy(bto, cb->buf+(ifrom), bsize_tmp * sizeof(char*)); \
        _bsize -= bsize_tmp; \
        if (_bsize) \
                yf_memcpy(bto+bsize_tmp, cb->buf, _bsize * sizeof(char*)); \
} while (0)

#define yf_cb_space_gwrite(cb, bto, ifrom, bsize) \
        _yf_cb_space_get_impl((cb), bto, ifrom, bsize)
        
#define yf_cb_space_gread(cb, bto, ifrom, bsize) \
        _yf_cb_space_get_impl((cb), bto, ifrom, bsize)

#endif

