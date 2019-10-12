/* See the COPYING file for license details.  */
#ifndef UNISTD_64
#define UNISTD_64

/* NOTE: We use `LL' here to try to force these constants to 64 bits
   so that the `%rax' register is loaded correctly.  Alas, this
   doesn't work.  */
#define __NR_read 0LL
#define __NR_write 1LL
#define __NR_brk 12LL
#define __NR_exit 60LL
#define __NR_time 201LL

#endif
