/* Linux (architecture-generic) error constants.  */
/* See the COPYING file for license details.  */
#ifndef LINUX_ERRNO_H
#define LINUX_ERRNO_H

/* To keep things simple, we only define a subset of error codes.
   This subset probably covers all possible error codes that this
   simple program may receive, but I cannot guarantee that.  */
#define EINTR           4
#define EIO             5
#define EBADF           9
#define EAGAIN          11
#define ENOMEM          12
#define EFAULT          14
#define EISDIR          21
#define EINVAL          22
#define EFBIG           27
#define ENOSPC          28
#define EPIPE           32
#define EWOULDBLOCK     EAGAIN

#endif
