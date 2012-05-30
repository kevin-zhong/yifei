#ifndef _YF_SHMEM_H
#define _YF_SHMEM_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

typedef struct
{
        char *    addr;
        size_t    size;
        yf_str_t  name;
        yf_log_t *log;
} yf_shm_t;


yf_int_t yf_shm_alloc(yf_shm_t *shm);
void yf_shm_free(yf_shm_t *shm);


#endif
