#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

#if !defined HAVE_SYS_SHM_H
#error no sys shm
#endif

#include <sys/ipc.h>
#include <sys/shm.h>

static yf_int_t  yf_named_shm_attach_in(yf_shm_t * shm
                , yf_int_t create, yf_int_t tmp_mem)
{
        int id = shm->key;

        if (id == YF_INVALID_SHM_KEY)
        {
                id = shmget(IPC_PRIVATE, shm->size, (SHM_R | SHM_W | IPC_CREAT));

                if (id == -1)
                {
                        yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                                      "shmget(%uz) failed", shm->size);
                        return YF_ERROR;
                }
                shm->key = id;
        }

        yf_log_debug1(YF_LOG_DEBUG, shm->log, 0, "shmget id: %d", id);

        shm->addr = shmat(id, NULL, 0);

        if (shm->addr == (void *)-1)
        {
                yf_log_error(YF_LOG_ALERT, shm->log, yf_errno, "shmat() failed");
                return YF_ERROR;
        }

        if (tmp_mem)
        {
                if (shmctl(id, IPC_RMID, NULL) == -1)
                {
                        yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                                      "shmctl(IPC_RMID) failed");
                        
                        shm->key = YF_INVALID_SHM_KEY;
                }
        }

        return YF_OK;
}


static void yf_named_shm_detach_in(yf_shm_t *shm, yf_int_t destory)
{
        if (shm->key && destory)
        {
                if (shmctl(shm->key, IPC_RMID, NULL) == -1)
                {
                        yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                                      "shmctl(IPC_RMID) failed");
                }
                shm->key = YF_INVALID_SHM_KEY;
        }
        if (shm->addr)
        {
                if (shmdt(shm->addr) == -1)
                {
                        yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                                "shmdt(%p) failed", shm->addr);
                }
        }
}

//if key==-1, will create a new
yf_int_t yf_named_shm_attach(yf_shm_t *shm)
{
        return yf_named_shm_attach_in(shm, shm->key == YF_INVALID_SHM_KEY, 0);
}

void yf_named_shm_detach(yf_shm_t *shm)
{
        yf_named_shm_detach_in(shm, 0);
}

void yf_named_shm_destory(yf_shm_t *shm)
{
        yf_named_shm_detach_in(shm, 1);
}


//shared mem
#if defined (HAVE_MAP_ANON)

yf_int_t
yf_shm_alloc(yf_shm_t *shm)
{
        shm->addr = (char *)mmap(NULL, shm->size,
                                 PROT_READ | PROT_WRITE,
                                 MAP_ANON | MAP_SHARED, -1, 0);

        if (shm->addr == MAP_FAILED)
        {
                yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                              "mmap(MAP_ANON|MAP_SHARED, %uz) failed", shm->size);
                return YF_ERROR;
        }

        return YF_OK;
}


void
yf_shm_free(yf_shm_t *shm)
{
        if (munmap((void *)shm->addr, shm->size) == -1)
        {
                yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                              "munmap(%p, %uz) failed", shm->addr, shm->size);
        }
}

#elif defined (HAVE_MAP_DEVZERO)

yf_int_t
yf_shm_alloc(yf_shm_t *shm)
{
        yf_fd_t fd;

        fd = open("/dev/zero", YF_FILE_RDWR);

        if (fd == -1)
        {
                yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                              "open(\"/dev/zero\") failed");
                return YF_ERROR;
        }

        shm->addr = (char *)mmap(NULL, shm->size, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, fd, 0);

        if (shm->addr == MAP_FAILED)
        {
                yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                              "mmap(/dev/zero, MAP_SHARED, %uz) failed", shm->size);
        }

        if (yf_close(fd) == -1)
        {
                yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                              "yf_close(\"/dev/zero\") failed");
        }

        return (shm->addr == MAP_FAILED) ? YF_ERROR : YF_OK;
}


void
yf_shm_free(yf_shm_t *shm)
{
        if (munmap((void *)shm->addr, shm->size) == -1)
        {
                yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                              "munmap(%p, %uz) failed", shm->addr, shm->size);
        }
}

#elif defined HAVE_SYS_SHM_H

#include <sys/ipc.h>
#include <sys/shm.h>


yf_int_t
yf_shm_alloc(yf_shm_t *shm)
{
        return yf_named_shm_attach_in(shm, 1, 1);
}


void
yf_shm_free(yf_shm_t *shm)
{
        yf_named_shm_detach_in(shm, 1);
}

#endif
