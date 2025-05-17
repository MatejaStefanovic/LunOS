#ifndef ERRNO_H
#define ERRNO_H

#define EPERM       1   // Operation not permitted
#define ENOENT      2   // No such file or directory
#define ESRCH       3   // No such process
#define EINTR       4   // Interrupted system call
#define EIO         5   // Input/output error
#define ENXIO       6   // No such device or address
#define E2BIG       7   // Argument list too long
#define ENOEXEC     8   // Exec format error
#define EBADF       9   // Bad file descriptor
#define ECHILD      10  // No child processes
#define EAGAIN      11  // Resource temporarily unavailable
#define ENOMEM      12  // Cannot allocate memory
#define EACCES      13  // Permission denied
#define EFAULT      14  // Bad address
#define EEXIST      17  // File exists
#define EOVERFLOW   75  // Value too large for defined data type

extern int errno;

#define set_errno(err) (errno = err)
#define get_errno() (errno)

#endif
