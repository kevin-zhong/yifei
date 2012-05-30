#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

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

        if (close(fd) == -1)
        {
                yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                              "close(\"/dev/zero\") failed");
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
        int id;

        id = shmget(IPC_PRIVATE, shm->size, (SHM_R | SHM_W | IPC_CREAT));

        if (id == -1)
        {
                yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                              "shmget(%uz) failed", shm->size);
                return YF_ERROR;
        }

        yf_log_debug1(YF_LOG_DEBUG, shm->log, 0, "shmget id: %d", id);

        shm->addr = shmat(id, NULL, 0);

        if (shm->addr == (void *)-1)
        {
                yf_log_error(YF_LOG_ALERT, shm->log, yf_errno, "shmat() failed");
        }

        if (shmctl(id, IPC_RMID, NULL) == -1)
        {
                yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                              "shmctl(IPC_RMID) failed");
        }

        return (shm->addr == (void *)-1) ? YF_ERROR : YF_OK;
}


void
yf_shm_free(yf_shm_t *shm)
{
        if (shmdt(shm->addr) == -1)
        {
                yf_log_error(YF_LOG_ALERT, shm->log, yf_errno,
                              "shmdt(%p) failed", shm->addr);
        }
}

#endif
