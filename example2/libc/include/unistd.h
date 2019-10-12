/* See the COPYING file for license details.  */
#ifndef UNISTD_H
#define UNISTD_H

/* OPERATING SYSTEM SPECIFIC DEFINITIONS */
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
/* END OPERATING SYSTEM SPECIFIC DEFINITIONS */

#include "stddef.h"

void _exit (int status);
long read (int fd, void *buf, unsigned long count);
long write (int fd, const void *buf, unsigned long count);
int brk (void *addr);
void *sbrk (long increment);

#endif
