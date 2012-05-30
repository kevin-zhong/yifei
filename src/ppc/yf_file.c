#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

ssize_t
yf_read_file(yf_file_t *file, char *buf, size_t size, off_t offset)
{
        ssize_t n;

        yf_log_debug4(YF_LOG_DEBUG, file->log, 0,
                      "read: %d, %p, %uz, %O", file->fd, buf, size, offset);

#if (HAVE_PREAD)

        n = pread(file->fd, buf, size, offset);

        if (n == -1)
        {
                yf_log_error(YF_LOG_CRIT, file->log, yf_errno,
                             "pread() \"%s\" failed", file->name.data);
                return YF_ERROR;
        }

#else

        if (file->sys_offset != offset)
        {
                if (lseek(file->fd, offset, SEEK_SET) == -1)
                {
                        yf_log_error(YF_LOG_CRIT, file->log, yf_errno,
                                     "lseek() \"%s\" failed", file->name.data);
                        return YF_ERROR;
                }

                file->sys_offset = offset;
        }

        n = read(file->fd, buf, size);

        if (n == -1)
        {
                yf_log_error(YF_LOG_CRIT, file->log, yf_errno,
                             "read() \"%s\" failed", file->name.data);
                return YF_ERROR;
        }

        file->sys_offset += n;

#endif

        file->offset += n;

        return n;
}


ssize_t
yf_write_file(yf_file_t *file, char *buf, size_t size, off_t offset)
{
        ssize_t n, written;

        yf_log_debug4(YF_LOG_DEBUG, file->log, 0,
                      "write: %d, %p, %uz, %O", file->fd, buf, size, offset);

        written = 0;

#if (HAVE_PWRITE)

        for (;; )
        {
                n = pwrite(file->fd, buf + written, size, offset);

                if (n == -1)
                {
                        yf_log_error(YF_LOG_CRIT, file->log, yf_errno,
                                     "pwrite() \"%s\" failed", file->name.data);
                        return YF_ERROR;
                }

                file->offset += n;
                written += n;

                if ((size_t)n == size)
                {
                        return written;
                }

                offset += n;
                size -= n;
        }

#else

        if (file->sys_offset != offset)
        {
                if (lseek(file->fd, offset, SEEK_SET) == -1)
                {
                        yf_log_error(YF_LOG_CRIT, file->log, yf_errno,
                                     "lseek() \"%s\" failed", file->name.data);
                        return YF_ERROR;
                }

                file->sys_offset = offset;
        }

        for (;; )
        {
                n = write(file->fd, buf + written, size);

                if (n == -1)
                {
                        yf_log_error(YF_LOG_CRIT, file->log, yf_errno,
                                     "write() \"%s\" failed", file->name.data);
                        return YF_ERROR;
                }

                file->offset += n;
                written += n;

                if ((size_t)n == size)
                {
                        return written;
                }

                size -= n;
        }
#endif
}


yf_fd_t
yf_open_tempfile(char *name, yf_uint_t persistent, yf_uint_t access)
{
        yf_fd_t fd;

        fd = open((const char *)name, O_CREAT | O_EXCL | O_RDWR,
                  access ? access : 0600);

        if (fd != -1 && !persistent)
        {
                unlink((const char *)name);
        }

        return fd;
}


#define YF_IOVS  8

ssize_t
yf_write_chain_to_file(yf_file_t *file, yf_chain_t *cl, off_t offset, yf_pool_t *pool)
{
        char *prev;
        size_t size;
        ssize_t n;
        yf_array_t vec;
        struct iovec *iov, iovs[YF_IOVS];

        /* use pwrite() if there is the only buf in a chain */

        if (cl->next == NULL)
        {
                return yf_write_file(file, cl->buf->pos,
                                     (size_t)(cl->buf->last - cl->buf->pos),
                                     offset);
        }

        vec.elts = iovs;
        vec.size = sizeof(struct iovec);
        vec.nalloc = YF_IOVS;
        vec.pool = pool;

        do {
                prev = NULL;
                iov = NULL;
                size = 0;

                vec.nelts = 0;

                /* create the iovec and coalesce the neighbouring bufs */

                while (cl && vec.nelts < YF_IOVS)
                {
                        if (prev == cl->buf->pos)
                        {
                                iov->iov_len += cl->buf->last - cl->buf->pos;
                        }
                        else {
                                iov = yf_array_push(&vec);
                                if (iov == NULL)
                                {
                                        return YF_ERROR;
                                }

                                iov->iov_base = (void *)cl->buf->pos;
                                iov->iov_len = cl->buf->last - cl->buf->pos;
                        }

                        size += cl->buf->last - cl->buf->pos;
                        prev = cl->buf->last;
                        cl = cl->next;
                }

                /* use pwrite() if there is the only iovec buffer */

                if (vec.nelts == 1)
                {
                        iov = vec.elts;
                        return yf_write_file(file, (char *)iov[0].iov_base,
                                             iov[0].iov_len, offset);
                }

                if (file->sys_offset != offset)
                {
                        if (lseek(file->fd, offset, SEEK_SET) == -1)
                        {
                                yf_log_error(YF_LOG_CRIT, file->log, yf_errno,
                                             "lseek() \"%s\" failed", file->name.data);
                                return YF_ERROR;
                        }

                        file->sys_offset = offset;
                }

                n = writev(file->fd, vec.elts, vec.nelts);

                if (n == -1)
                {
                        yf_log_error(YF_LOG_CRIT, file->log, yf_errno,
                                     "writev() \"%s\" failed", file->name.data);
                        return YF_ERROR;
                }

                if ((size_t)n != size)
                {
                        yf_log_error(YF_LOG_CRIT, file->log, 0,
                                     "writev() \"%s\" has written only %z of %uz",
                                     file->name.data, n, size);
                        return YF_ERROR;
                }

                file->sys_offset += n;
                file->offset += n;
        } while (cl);

        return n;
}


yf_int_t
yf_set_file_time(char *name, yf_fd_t fd, time_t s)
{
        struct timeval tv[2];

        tv[0].tv_sec = yf_time();
        tv[0].tv_usec = 0;
        tv[1].tv_sec = s;
        tv[1].tv_usec = 0;

        if (utimes((char *)name, tv) != -1)
        {
                return YF_OK;
        }

        return YF_ERROR;
}


yf_int_t
yf_create_file_mapping(yf_file_mapping_t *fm)
{
        fm->fd = yf_open_file(fm->name, YF_FILE_RDWR, YF_FILE_TRUNCATE,
                              YF_FILE_DEFAULT_ACCESS);
        if (fm->fd == YF_INVALID_FILE)
        {
                yf_log_error(YF_LOG_CRIT, fm->log, yf_errno,
                             yf_open_file_n " \"%s\" failed", fm->name);
                return YF_ERROR;
        }

        if (ftruncate(fm->fd, fm->size) == -1)
        {
                yf_log_error(YF_LOG_CRIT, fm->log, yf_errno,
                             "ftruncate() \"%s\" failed", fm->name);
                goto failed;
        }

        fm->addr = mmap(NULL, fm->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fm->fd, 0);
        if (fm->addr != MAP_FAILED)
        {
                return YF_OK;
        }

        yf_log_error(YF_LOG_CRIT, fm->log, yf_errno,
                     "mmap(%uz) \"%s\" failed", fm->size, fm->name);

failed:

        if (yf_close_file(fm->fd) == YF_FILE_ERROR)
        {
                yf_log_error(YF_LOG_ALERT, fm->log, yf_errno,
                             yf_close_file_n " \"%s\" failed", fm->name);
        }

        return YF_ERROR;
}


void
yf_close_file_mapping(yf_file_mapping_t *fm)
{
        if (munmap(fm->addr, fm->size) == -1)
        {
                yf_log_error(YF_LOG_CRIT, fm->log, yf_errno,
                             "munmap(%uz) \"%s\" failed", fm->size, fm->name);
        }

        if (yf_close_file(fm->fd) == YF_FILE_ERROR)
        {
                yf_log_error(YF_LOG_ALERT, fm->log, yf_errno,
                             yf_close_file_n " \"%s\" failed", fm->name);
        }
}


yf_int_t
yf_open_dir(yf_str_t *name, yf_dir_t *dir)
{
        dir->dir = opendir((const char *)name->data);

        if (dir->dir == NULL)
        {
                return YF_ERROR;
        }

        dir->valid_info = 0;

        return YF_OK;
}


yf_int_t
yf_read_dir(yf_dir_t *dir)
{
        dir->de = readdir(dir->dir);

        if (dir->de)
        {
#if (HAVE_D_TYPE)
                dir->type = dir->de->d_type;
#else
                dir->type = 0;
#endif
                return YF_OK;
        }

        return YF_ERROR;
}


yf_err_t
yf_trylock_fd(yf_fd_t fd)
{
        struct flock fl;

        fl.l_start = 0;
        fl.l_len = 0;
        fl.l_pid = 0;
        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;

        if (fcntl(fd, F_SETLK, &fl) == -1)
        {
                return yf_errno;
        }

        return 0;
}


yf_err_t
yf_lock_fd(yf_fd_t fd)
{
        struct flock fl;

        fl.l_start = 0;
        fl.l_len = 0;
        fl.l_pid = 0;
        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;

        if (fcntl(fd, F_SETLKW, &fl) == -1)
        {
                return yf_errno;
        }

        return 0;
}


yf_err_t
yf_unlock_fd(yf_fd_t fd)
{
        struct flock fl;

        fl.l_start = 0;
        fl.l_len = 0;
        fl.l_pid = 0;
        fl.l_type = F_UNLCK;
        fl.l_whence = SEEK_SET;

        if (fcntl(fd, F_SETLK, &fl) == -1)
        {
                return yf_errno;
        }

        return 0;
}


yf_int_t
yf_create_path(yf_file_t *file, yf_path_t *path)
{
        size_t pos;
        yf_err_t err;
        yf_uint_t i;

        pos = path->name.len;

        for (i = 0; i < 3; i++)
        {
                if (path->level[i] == 0)
                {
                        break;
                }

                pos += path->level[i] + 1;

                file->name.data[pos] = '\0';

                yf_log_debug1(YF_LOG_DEBUG, file->log, 0,
                              "temp file: \"%s\"", file->name.data);

                if (yf_create_dir(file->name.data, 0700) == YF_FILE_ERROR)
                {
                        err = yf_errno;
                        if (err != YF_EEXIST)
                        {
                                yf_log_error(YF_LOG_CRIT, file->log, err,
                                             yf_create_dir_n " \"%s\" failed",
                                             file->name.data);
                                return YF_ERROR;
                        }
                }

                file->name.data[pos] = '/';
        }

        return YF_OK;
}


yf_err_t
yf_create_full_path(char *dir, yf_uint_t access)
{
        char *p, ch;
        yf_err_t err;

        err = 0;
        p = dir + 1;

        for (/* void */; *p; p++)
        {
                ch = *p;

                if (ch != '/')
                {
                        continue;
                }

                *p = '\0';

                if (yf_create_dir(dir, access) == YF_FILE_ERROR)
                {
                        err = yf_errno;

                        switch (err)
                        {
                        case YF_EEXIST:
                                err = 0;
                        case YF_EACCES:
                                break;

                        default:
                                return err;
                        }
                }

                *p = '/';
        }

        return err;
}


