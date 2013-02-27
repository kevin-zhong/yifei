#ifndef _YF_FILE_H
#define _YF_FILE_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

typedef int yf_fd_t;
typedef struct stat yf_file_info_t;

typedef struct
{
        char *    name;
        size_t    size;
        void *    addr;
        yf_log_t *log;
        yf_fd_t   fd;
}
yf_file_mapping_t;


struct yf_file_s
{
        yf_fd_t        fd;
        yf_str_t       name;
        yf_file_info_t info;

        off_t          offset;
        off_t          sys_offset;

        yf_log_t *     log;

        unsigned       valid_info : 1;
        unsigned       directio : 1;
};


typedef struct
{
        DIR *          dir;
        struct dirent *de;
        struct stat    info;

        yf_log_t *     log;

        unsigned       type : 8;
        unsigned       valid_info : 1;
}
yf_dir_t;


typedef struct
{
        yf_str_t  name;
        size_t    level[3];
        void *    data;

        yf_log_t *log;
        yf_uint_t line;
}
yf_path_t;


#define YF_INVALID_FILE         -1
#define YF_FILE_ERROR           -1



#define yf_open_file(name, mode, create, access)                            \
        open((const char *)name, mode | create, access)

#define yf_open_file_n         "open()"

#define YF_FILE_RDONLY          O_RDONLY
#define YF_FILE_WRONLY          O_WRONLY
#define YF_FILE_RDWR            O_RDWR
#define YF_FILE_CREATE_OR_OPEN  O_CREAT
#define YF_FILE_OPEN            0
#define YF_FILE_TRUNCATE        O_CREAT | O_TRUNC
#define YF_FILE_APPEND          O_WRONLY | O_APPEND
#define YF_FILE_NONBLOCK        O_NONBLOCK

#define YF_FILE_DEFAULT_ACCESS  0644
#define YF_FILE_OWNER_ACCESS    0600


#define yf_close_file           yf_close
#define yf_close_file_n         "yf_close()"


#define yf_delete_file(name)    unlink((const char *)name)
#define yf_delete_file_n        "unlink()"


yf_fd_t yf_open_tempfile(char *name, yf_uint_t persistent, yf_uint_t access);
#define yf_open_tempfile_n      "open()"


ssize_t yf_read_file(struct yf_file_s *file, char *buf, size_t size, off_t offset);
#if (HAVE_PREAD)
#define yf_read_file_n          "pread()"
#else
#define yf_read_file_n          "read()"
#endif

ssize_t yf_write_file(struct yf_file_s *file, char *buf, size_t size, off_t offset);



#define yf_linefeed(p)          *p++ = '\n';
#define YF_LINEFEED_SIZE        1


#define yf_rename_file(o, n)    rename((const char *)o, (const char *)n)
#define yf_rename_file_n        "rename()"


#define yf_change_file_access(n, a) chmod((const char *)n, a)
#define yf_change_file_access_n "chmod()"


yf_int_t yf_set_file_time(char *name, yf_fd_t fd, time_t s);
#define yf_set_file_time_n      "utimes()"


#define yf_file_info(file, sb)  stat((const char *)file, sb)
#define yf_file_info_n          "stat()"

#define yf_fd_info(fd, sb)      fstat(fd, sb)
#define yf_fd_info_n            "fstat()"

#define yf_link_info(file, sb)  lstat((const char *)file, sb)
#define yf_link_info_n          "lstat()"

#define yf_is_dir(sb)           (S_ISDIR((sb)->st_mode))
#define yf_is_file(sb)          (S_ISREG((sb)->st_mode))
#define yf_is_link(sb)          (S_ISLNK((sb)->st_mode))
#define yf_is_exec(sb)          (((sb)->st_mode & S_IXUSR) == S_IXUSR)
#define yf_file_access(sb)      ((sb)->st_mode & 0777)
#define yf_file_size(sb)        (sb)->st_size
#define yf_file_fs_size(sb)     ((sb)->st_blocks * 512)
#define yf_file_mtime(sb)       (sb)->st_mtime
#define yf_file_uniq(sb)        (sb)->st_ino


yf_int_t yf_create_file_mapping(yf_file_mapping_t *fm);
void yf_close_file_mapping(yf_file_mapping_t *fm);


#define yf_realpath(p, r)       realpath((char *)p, (char *)r)
#define yf_realpath_n           "realpath()"
#define yf_getcwd(buf, size)    (getcwd((char *)buf, size) != NULL)
#define yf_getcwd_n             "getcwd()"
#define yf_path_separator(c)    ((c) == '/')

#define YF_MAX_PATH             PATH_MAX

#define YF_DIR_MASK_LEN         0


yf_int_t yf_open_dir(yf_str_t *name, yf_dir_t *dir);
#define yf_open_dir_n           "opendir()"


#define yf_close_dir(d)         closedir((d)->dir)
#define yf_close_dir_n          "closedir()"


yf_int_t yf_read_dir(yf_dir_t *dir);
#define yf_read_dir_n           "readdir()"


#define yf_create_dir(name, access) mkdir((const char *)name, access)
#define yf_create_dir_n         "mkdir()"


#define yf_delete_dir(name)     rmdir((const char *)name)
#define yf_delete_dir_n         "rmdir()"


#define yf_dir_access(a)        (a | (a & 0444) >> 2)


#define yf_de_name(dir)         ((char *)(dir)->de->d_name)
#if (HAVE_D_NAMLEN)
#define yf_de_namelen(dir)      (dir)->de->d_namlen
#else
#define yf_de_namelen(dir)      yf_strlen((dir)->de->d_name)
#endif

static inline yf_int_t
yf_de_info(char *name, yf_dir_t *dir)
{
        dir->type = 0;
        return stat((const char *)name, &dir->info);
}

#define yf_de_info_n            "stat()"
#define yf_de_link_info(name, dir)  lstat((const char *)name, &(dir)->info)
#define yf_de_link_info_n       "lstat()"

#if (HAVE_D_TYPE)

/*
 * some file systems (e.g. XFS on Linux and CD9660 on FreeBSD)
 * do not set dirent.d_type
 */
#define yf_de_is_dir(dir)                                                   \
        (((dir)->type) ? ((dir)->type == DT_DIR) : (S_ISDIR((dir)->info.st_mode)))
#define yf_de_is_file(dir)                                                  \
        (((dir)->type) ? ((dir)->type == DT_REG) : (S_ISREG((dir)->info.st_mode)))
#define yf_de_is_link(dir)                                                  \
        (((dir)->type) ? ((dir)->type == DT_LNK) : (S_ISLNK((dir)->info.st_mode)))
#else
#define yf_de_is_dir(dir)       (S_ISDIR((dir)->info.st_mode))
#define yf_de_is_file(dir)      (S_ISREG((dir)->info.st_mode))
#define yf_de_is_link(dir)      (S_ISLNK((dir)->info.st_mode))
#endif

#define yf_de_access(dir)       (((dir)->info.st_mode) & 0777)
#define yf_de_size(dir)         (dir)->info.st_size
#define yf_de_fs_size(dir)      ((dir)->info.st_blocks * 512)
#define yf_de_mtime(dir)        (dir)->info.st_mtime


yf_err_t yf_trylock_fd(yf_fd_t fd);
yf_err_t yf_lock_fd(yf_fd_t fd);
yf_err_t yf_unlock_fd(yf_fd_t fd);

#define yf_trylock_fd_n         "fcntl(F_SETLK, F_WRLCK)"
#define yf_lock_fd_n            "fcntl(F_SETLKW, F_WRLCK)"
#define yf_unlock_fd_n          "fcntl(F_SETLK, F_UNLCK)"


#define yf_stderr               STDERR_FILENO
#define yf_set_stderr(fd)       dup2(fd, STDERR_FILENO)
#define yf_set_stderr_n         "dup2(STDERR_FILENO)"


yf_int_t yf_create_path(struct yf_file_s *file, yf_path_t *path);
yf_err_t yf_create_full_path(char *dir, yf_uint_t access);


#endif /* _YF_FILE_H */
