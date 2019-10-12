/* An implementation of low-level Unix I/O functions for use
   internally by a program that virtualizes multiple processes.  */

/* Write data to an output file.  */

/* Read data from an output file.  */

/* Create a file descriptor.  An internal circular buffer for that
   file descriptor is allocated.  */

/*
fcntl.h
sys/stat.h
unistd.h
*/

int creat(const char *filename, mode_t mode)
{
}

int open(const char *file, int mode, int permissions)
{
}

ssize_t read(int fd, void *buffer, size_t length)
{
}

int write(int file, const void *buffer, size_t count)
{
}

int close(int fd)
{
}

int dup(int fd)
{
}

int dup2(int existing_handle, int new_handle)
{
}

int stat(const char *file, struct stat *sbuf)
{
}

int link(const char *exists, const char *new)
{
}

off_t lseek(int fd, off_t offset, int whence)
{
}

int mkfifo(const char *path, mode_t mode)
{
}

int pipe(int filedes[2])
{
}

pid_t fork()
{
}

/*

popen? pclose?

On embedded systems, the entire system is a single "application".
Therefore, you do not run any other applications.  Typically, your
entire program is all the code the system will ever run.  Your code
directly interfaces with firmware rather than going through the
operating system.  However, if your program is the operating system,
then this is a different situation.

In that case, your program provides a limited form of an operating
system.  Applications separation is not enforced with memory
protection.  Applications will typically multitask cooperatively
rather than pre-emptively.  And multitasking is seldom done because
the system has so little memory.  Enforcement of restrictions is
seldom done on an embedded system.

*/
