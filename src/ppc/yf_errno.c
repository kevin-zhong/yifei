#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

#define  YF_MAX_ERR_NO  256

static yf_str_t* yf_err_str_list;
static yf_err_t  yf_err_no_end;
static const yf_str_t yf_unknown_err_str = yf_str("Unknown error");

const yf_str_t* yf_strerror(yf_err_t err)
{
        return  (yf_uint_t)err >= yf_err_no_end ? &yf_unknown_err_str : &yf_err_str_list[err];
}

yf_int_t yf_strerror_init(void)
{
        yf_err_no_end = 0;
        
        yf_int_t list_size = YF_MAX_ERR_NO;
        yf_err_str_list = yf_alloc(sizeof(yf_str_t) * list_size);
        CHECK_RV(yf_err_str_list == NULL, YF_ERROR);
        
        for (yf_err_no_end = 0; yf_err_no_end < YF_MAX_ERR_NO; ++yf_err_no_end)
        {
                yf_errno = 0;
                char* p_errstr = strerror(yf_err_no_end);
                
                if (yf_err_no_end >= list_size)
                {
                        list_size = list_size*3/2;
                        yf_err_str_list = yf_realloc(yf_err_str_list, list_size);
                        CHECK_RV(yf_err_str_list == NULL, YF_ERROR);
                }

                if (yf_errno == YF_EINVAL
                        || p_errstr == NULL
                        || yf_strncmp(p_errstr, "Unknown error", 13) == 0)
                {
                        yf_err_str_list[yf_err_no_end] = yf_unknown_err_str;
                        continue;
                }

                size_t  len = yf_strlen(p_errstr);
                char* str_alloc = yf_alloc(len + 1);
                CHECK_RV(str_alloc == NULL, YF_ERROR);
                
                yf_strncpy(str_alloc, p_errstr, len);

                yf_err_str_list[yf_err_no_end].data = str_alloc;
                yf_err_str_list[yf_err_no_end].len = len;
        }

        return  YF_OK;
}


