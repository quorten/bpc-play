/* See the COPYING file for license details.  */
#ifndef ERRNO_H
#define ERRNO_H

extern int errno;

#if OS_LINUX
#include "sysc/linux/errno.h"
#else
#error "Undefined platform"
#endif

#endif
