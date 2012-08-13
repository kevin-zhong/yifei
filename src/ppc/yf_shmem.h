#ifndef _YF_SHMEM_H
#define _YF_SHMEM_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

typedef struct
{
        yf_int_t  key;
        char *    addr;
        size_t    size;
        yf_str_t  name;
        yf_log_t *log;
} 
yf_shm_t;

#define YF_INVALID_SHM_KEY -1

//if key==-1, will create a new
yf_int_t yf_named_shm_attach(yf_shm_t *shm);
void yf_named_shm_detach(yf_shm_t *shm);

//if addr != NULL, will detach after destory
void yf_named_shm_destory(yf_shm_t *shm);


yf_int_t yf_shm_alloc(yf_shm_t *shm);
void yf_shm_free(yf_shm_t *shm);


#endif
