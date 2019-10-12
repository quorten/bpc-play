/* See the COPYING file for license details.  */
#ifndef UNISTD_32
#define UNISTD_32

#define __NR_Linux 4000
#define __NR_syscall (__NR_Linux + 0)
#define __NR_exit (__NR_Linux + 1)
#define __NR_read (__NR_Linux + 3)
#define __NR_write (__NR_Linux + 4)
#define __NR_time (__NR_Linux +  13)
#define __NR_brk (__NR_Linux + 45)

#endif
