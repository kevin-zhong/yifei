#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

yf_int_t  yf_circular_buf_init(yf_circular_buf_t* cb, yf_s32_t buf_size, yf_log_t * log)
{
        yf_memzero(cb, sizeof(yf_circular_buf_t));
        cb->buf_size = yf_align_2pow(buf_size);
        cb->buf_size_pow = yf_bit_cnt(cb->buf_size) - 1;
        cb->mpool = yf_create_pool(yf_pagesize >> 1, log);
        
        CHECK_RV(cb->mpool == NULL, YF_ERROR);
        return  YF_OK;
}

void  yf_circular_buf_destory(yf_circular_buf_t* cb)
{
        yf_u32_t i1 = 0;
        if (cb->mpool)
        {
                yf_destroy_pool(cb->mpool);
                cb->mpool = NULL;
        }

        for ( i1 = 0; i1 < cb->buf_used; i1++ )
        {
                assert(cb->buf[i1]);
                yf_free(cb->buf[i1]);
        }
        yf_memzero(cb, sizeof(yf_circular_buf_t));
}


#define _yf_cb_bufs_alloc(cb, size) yf_palloc((cb)->mpool, sizeof(char**) * \
                (yf_circular_buf_bsize(cb, size) + 1));

#define _yf_cb_preprocess_cursor(i, o, cb) \
        if (o == cb->buf_size) { \
                i = yf_cb_increase(i, cb->buf_used); \
                o = 0; \
        }

//ret read size
yf_s32_t  yf_cb_space_read(yf_circular_buf_t* cb, yf_s32_t bytes
                , char*** rbufs, yf_s32_t* roffset, yf_int_t flags)
{
        char** b = NULL;
        yf_s32_t  rbytes = 0;
        yf_s16_t  rbsize = 0;
        yf_s16_t  rindex = cb->cursor_index, tindex = cb->tail_index;
        yf_s32_t  rest_rsize = yf_circular_buf_rest_rsize(cb);

        CHECK_RV(bytes <= 0, bytes);

        if (*rbufs == NULL)
                *rbufs = _yf_cb_bufs_alloc(cb, bytes);
        b = *rbufs;
        *roffset = cb->cursor_offset;

        _yf_cb_preprocess_cursor(rindex, *roffset, cb);

        if (unlikely(rest_rsize <= bytes))
        {
                rbytes = rest_rsize;
                
                rbsize = yf_cb_diff(tindex, rindex, cb->buf_used, YF_CBR) + 1;

                yf_cb_space_gread(cb, b, rindex, rbsize);
                
                if (flags != YF_READ_PEEK)
                {
                        cb->cursor_index = tindex;
                        cb->cursor_offset = cb->tail_offset;
                }
                return rbytes;
        }

        *b = cb->buf[rindex];
        rbytes = cb->buf_size - *roffset;
        if (likely(rbytes >= bytes))
        {
                if (flags != YF_READ_PEEK)
                {
                        cb->cursor_index = rindex;
                        cb->cursor_offset = *roffset + bytes;//may cursor_offset == buf_size
                }
                return bytes;
        }
        ++b;
        assert(tindex != rindex);

        //rest num
        rbytes = bytes - rbytes;
        rbsize = yf_circular_buf_bsize(cb, rbytes);

        if (flags != YF_READ_PEEK)
        {
                cb->cursor_index = yf_cb_add(rindex, rbsize, cb->buf_used);
                cb->cursor_offset = yf_cb_rest_mod(rbytes, cb->buf_size);
        }        

        rindex = yf_cb_increase(rindex, cb->buf_used);
        yf_cb_space_gread(cb, b, rindex, rbsize);
        
        return  bytes;
}


static  void  _yf_cb_space_reoder(yf_circular_buf_t* cb)
{
        yf_s32_t  wbsize = 0;
        yf_s16_t  hindex = cb->head_index;
        if (unlikely(hindex == 0))
                return;
        
        char** buf_swap = yf_palloc(cb->mpool, hindex * sizeof(char*));
        assert(buf_swap);
        yf_memcpy(buf_swap, cb->buf, hindex * sizeof(char*));

        wbsize = cb->buf_used - hindex;
        yf_memmove(cb->buf, cb->buf+hindex, wbsize * sizeof(char*));
        yf_memcpy(cb->buf + wbsize, buf_swap, hindex * sizeof(char*));

        cb->cursor_index = yf_cb_sub(cb->cursor_index, hindex, cb->buf_used);
        cb->tail_index = yf_cb_sub(cb->tail_index, hindex, cb->buf_used);
        cb->head_index = 0;
}


//ret new free_wsize
yf_s32_t  yf_cb_space_enlarge(yf_circular_buf_t* cb, yf_s32_t bytes)
{
        yf_s32_t  free_wsize = yf_circular_buf_free_wsize(cb);
        yf_s32_t  wbsize = 0, wbsize_r;

        if (free_wsize < bytes && cb->buf_used < YF_CIRCULAR_BUF_MAX_NUM)
        {
                //try to alloc more buf
                //1-reorder buf index
                if (cb->head_index)
                        _yf_cb_space_reoder(cb);

                //2-expand
                wbsize = yf_circular_buf_bsize(cb, bytes - free_wsize);
                wbsize = yf_min(wbsize, YF_CIRCULAR_BUF_MAX_NUM - cb->buf_used);
                wbsize_r = wbsize;
                while (wbsize)
                {
                        cb->buf[cb->buf_used] = yf_alloc(cb->buf_size);
                        if (cb->buf[cb->buf_used] == NULL)
                                break;
                        cb->buf_used++;
                        wbsize--;
                }

                return free_wsize + ((wbsize_r-wbsize) << cb->buf_size_pow);
        }        
        else
                return free_wsize;
}


#define _yf_cb_space_write_tail(cb, free_wsize, bytes) \
        yf_s32_t  _new_wsize = bytes - (free_wsize - yf_circular_buf_tailed_wsize(cb)); \
        if (_new_wsize > 0) { \
                __yf_circular_buf_advance_front(cb->tail_index, cb->tail_offset, \
                                _new_wsize, cb); \
        }


//ret writed space
yf_s32_t  yf_cb_space_write_alloc(yf_circular_buf_t* cb, yf_s32_t bytes
                , char*** rbufs, yf_s32_t* woffset)
{
        char** b = NULL;
        yf_s32_t  free_wsize = yf_circular_buf_free_wsize(cb);
        yf_s32_t  wbytes = 0;
        yf_s16_t  wbsize = 0;
        yf_s16_t  windex = cb->cursor_index;

        CHECK_RV(bytes <= 0, bytes);
        
        if (unlikely(free_wsize < bytes))
        {
                free_wsize = yf_cb_space_enlarge(cb, bytes);
                windex = cb->cursor_index;
        }

        bytes = yf_min(bytes, free_wsize);

        //same with read impl
        if (*rbufs == NULL)
                *rbufs = _yf_cb_bufs_alloc(cb, bytes);
        b = *rbufs;
        *woffset = cb->cursor_offset;

        _yf_cb_preprocess_cursor(windex, *woffset, cb);
        
        wbytes = cb->buf_size - *woffset;
        
        *b = cb->buf[windex];
        if (wbytes >= bytes)
        {
                return bytes;
        }
        ++b;

        //rest num
        wbytes = bytes - wbytes;
        wbsize = yf_circular_buf_bsize(cb, wbytes);

        windex = yf_cb_increase(windex, cb->buf_used);
        yf_cb_space_gwrite(cb, b, windex, wbsize);
        return bytes;
}


void yf_cb_space_write_bytes(yf_circular_buf_t* cb, yf_s32_t wreal_len)
{
        yf_s32_t  free_wsize = yf_circular_buf_free_wsize(cb);
        
        //process tail i+o
        _yf_cb_space_write_tail(cb, free_wsize, wreal_len);
        
        yf_int_t ret = yf_cb_fseek(cb, wreal_len, YF_SEEK_CUR);
        assert(ret == YF_OK);
}


/*
* shrink
*/
void  yf_circular_buf_shrink(yf_circular_buf_t* cb, yf_log_t * log)
{
        yf_s16_t  shrink_num;
        yf_s16_t  shrink_start;
        yf_s32_t  tailed_wsize;
        yf_pool_t* pool;

        if (++cb->shrink_cnt < 32)
        {
                yf_reset_pool(cb->mpool);
                return;
        }
        
        cb->shrink_cnt = 0;
        
        pool = yf_create_pool(yf_pagesize >> 1, log);
        if (pool)
        {
                yf_destroy_pool(cb->mpool);
                cb->mpool = pool;
        }
        else
                yf_reset_pool(cb->mpool);
        
        if (cb->buf_used <= 2)
                return;
        
        shrink_num = yf_max(1, cb->buf_used>>3);
        tailed_wsize = yf_circular_buf_tailed_wsize(cb);

        tailed_wsize -= (cb->buf_size - cb->tail_offset);
        assert(yf_mod(tailed_wsize, cb->buf_size) == 0);

        shrink_num = yf_min(shrink_num, (tailed_wsize >> cb->buf_size_pow));
        if (shrink_num == 0)
                return;

        _yf_cb_space_reoder(cb);
        assert(cb->head_index == 0);

        shrink_start = cb->buf_used - shrink_num;

        yf_log_debug(YF_LOG_DEBUG, log, 0, 
                        "shrink buf num=%d, shrink_start=%d, org used size=%d", 
                        shrink_num, shrink_start, cb->buf_used);

        while (shrink_start < cb->buf_used)
        {
                yf_free(cb->buf[shrink_start]);
                ++shrink_start;
        }
        
        cb->buf_used -= shrink_num;
}




//if offset=0, use now cursor as start
yf_int_t  yf_cb_fhead_set(yf_circular_buf_t* cb, yf_s32_t offset)
{
        CHECK_RV(offset < 0, YF_ERROR);
        
        if (offset == 0)
        {
                cb->head_index = cb->cursor_index;
                cb->head_offset = cb->cursor_offset;
                return YF_OK;
        }
        
        yf_s32_t fsize = yf_cb_fsize(cb);
        if (offset > fsize)
                return YF_ERROR;
        
        __yf_circular_buf_advance_front(cb->head_index, cb->head_offset, offset, cb);

        //check cursor pos
        yf_s32_t  cpos = yf_cb_ftell(cb);
        if (cpos < 0 || cpos > yf_cb_fsize(cb))
        {
                cb->cursor_index = cb->head_index;
                cb->cursor_offset = cb->head_offset;
        }
        return YF_OK;
}


yf_int_t yf_cb_ftruncate(yf_circular_buf_t* cb, yf_s32_t size)
{
        CHECK_RV(size < 0, YF_ERROR);
        
        yf_s32_t  osize = yf_cb_fsize(cb);
        yf_s32_t  dsize = size - osize;

        if (dsize == 0)
                return YF_OK;
        else if (dsize > 0)
        {
                yf_s32_t  more_bytes = size - yf_cb_ftell(cb);
                yf_s32_t  free_wsize = yf_cb_space_enlarge(cb, more_bytes);
                
                if (free_wsize < more_bytes)
                        return YF_ERROR;

                //process tail i+o
                _yf_cb_space_write_tail(cb, free_wsize, more_bytes);
                return YF_OK;
        }

        dsize = -dsize;
        __yf_circular_buf_advance_back(cb->tail_index, cb->tail_offset, dsize, cb);
        yf_s32_t cpos = yf_cb_ftell(cb);
        
        if (cpos < 0 || cpos > size)
        {
                cb->cursor_index = cb->tail_index;
                cb->cursor_offset = cb->tail_offset;
        }        
        return YF_OK;
}


yf_int_t yf_cb_fseek(yf_circular_buf_t* cb, yf_s32_t offset, yf_int_t fromwhere)
{
        if (likely(offset == 0))
        {
                switch (fromwhere)
                {
                        case YF_SEEK_SET:
                                cb->cursor_index = cb->head_index;
                                cb->cursor_offset = cb->head_offset;
                                break;
                        case YF_SEEK_CUR:
                                break;
                        case YF_SEEK_END:
                                cb->cursor_index = cb->tail_index;
                                cb->cursor_offset = cb->tail_offset;
                                break;
                        default:
                                return YF_ERROR;
                }
                return  YF_OK;
        }

        yf_s32_t offset_abs = offset;
        yf_s32_t fsize = yf_cb_fsize(cb);
        
        switch (fromwhere)
        {
                case YF_SEEK_SET:
                        break;
                case YF_SEEK_CUR:
                        offset_abs += yf_cb_ftell(cb);
                        break;
                case YF_SEEK_END:
                        offset_abs += fsize;
                        break;
                default:
                        return YF_ERROR;
        }

        if (offset_abs < 0 || offset_abs > fsize)
                return YF_ERROR;

        cb->cursor_index = cb->head_index;
        cb->cursor_offset = cb->head_offset;
        __yf_circular_buf_advance_front(cb->cursor_index, cb->cursor_offset, offset_abs, cb);
        return YF_OK;
}



yf_s32_t yf_cb_fread(yf_circular_buf_t* cb, yf_s32_t bytes, yf_int_t flags, char** rbuf)
{
        yf_s32_t  rbytes = 0;
        yf_s32_t  roffset = cb->cursor_offset;
        yf_s16_t  rindex = cb->cursor_index, tindex = cb->tail_index;

        yf_s32_t  rest_rsize = yf_circular_buf_rest_rsize(cb);
        bytes = yf_min(rest_rsize, bytes);
        CHECK_RV(bytes <= 0, bytes);

        _yf_cb_preprocess_cursor(rindex, roffset, cb);

        rbytes = cb->buf_size - roffset;
        if (likely(rbytes >= bytes))
        {
                *rbuf = cb->buf[rindex] + roffset;
                
                if (flags != YF_READ_PEEK)
                {
                        cb->cursor_index = rindex;
                        cb->cursor_offset = roffset + bytes;//may cursor_offset == buf_size
                }
                return bytes;
        }

        char* b = NULL;

        b = yf_palloc(cb->mpool, bytes);
        assert(b);
        *rbuf = b;

        b = yf_cpymem(b, cb->buf[rindex] + roffset, rbytes);
        rindex = yf_cb_increase(rindex, cb->buf_used);

        //rest num
        rbytes = bytes - rbytes;
        while (1)
        {
                if (likely(rbytes <= cb->buf_size))
                {
                        b = yf_cpymem(b, cb->buf[rindex], rbytes);
                        
                        if (flags != YF_READ_PEEK)
                        {
                                cb->cursor_index = rindex;
                                cb->cursor_offset = rbytes;
                        }
                        break;
                }
                else {
                        b = yf_cpymem(b, cb->buf[rindex], cb->buf_size);
                        rindex = yf_cb_increase(rindex, cb->buf_used);
                        rbytes -= cb->buf_size;
                }
        }
        return  bytes;
}


yf_s32_t yf_cb_fwrite(yf_circular_buf_t* cb, char* buf, yf_s32_t bytes)
{
        yf_s32_t  wbytes = 0;
        yf_s32_t  free_wsize = yf_circular_buf_free_wsize(cb);
        yf_s32_t  woffset = cb->cursor_offset;
        yf_s16_t  windex = cb->cursor_index;

        CHECK_RV(bytes <= 0, bytes);
        
        if (unlikely(free_wsize < bytes))
        {
                free_wsize = yf_cb_space_enlarge(cb, bytes);
                windex = cb->cursor_index;
        }

        bytes = yf_min(bytes, free_wsize);
        //process tail i+o
        _yf_cb_space_write_tail(cb, free_wsize, bytes);

        _yf_cb_preprocess_cursor(windex, woffset, cb);

        wbytes = cb->buf_size - woffset;
        if (likely(wbytes >= bytes))
        {
                yf_memcpy(cb->buf[windex] + woffset, buf, bytes);
                cb->cursor_index = windex;
                cb->cursor_offset = woffset + bytes;
                return  bytes;
        }

        yf_memcpy(cb->buf[windex] + woffset, buf, wbytes);
        windex = yf_cb_increase(windex, cb->buf_used);
        buf += wbytes;

        //rest num
        wbytes = bytes - wbytes;
        while (1)
        {
                if (likely(wbytes <= cb->buf_size))
                {
                        yf_memcpy(cb->buf[windex], buf, wbytes);
                        
                        cb->cursor_index = windex;
                        cb->cursor_offset = wbytes;
                        break;
                }
                else {
                        yf_memcpy(cb->buf[windex], buf, cb->buf_size);
                        windex = yf_cb_increase(windex, cb->buf_used);
                        wbytes -= cb->buf_size;
                        buf += cb->buf_size;
                }
        }
        
        return  bytes;
}


