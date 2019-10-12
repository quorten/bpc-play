/* System call function definitions.  */
/* See the COPYING file for license details.  */
#ifndef SYSC_H
#define SYSC_H

void exit (int status);
long read (int fd, void *buf, unsigned long count);
long write (int fd, const void *buf, unsigned long count);

#endif
