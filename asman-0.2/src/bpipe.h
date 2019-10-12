/* 2019 */
#ifndef BPIPE_H
#define BPIPE_H

/* BPIPE_BUFSIZE must be a power of two.  */
#define BPIPE_BUFSIZE 512

/* N.B. As has been discussed elsewhere, there are two general ways to
   implement these kinds of data structures, either using pointers or
   indexes.  We use indexes because they are smaller, save memory, and
   are more compatible with early and limited CPUs.  Of course, this
   comes at the expense of being slower on later CPUs that have wide
   enough registers for full hardware address arithmetic.  But, of
   course, that is beyond the scope of early boot software.  */
struct BPipe_tag
{
	unsigned char buf[BPIPE_BUFSIZE];
	unsigned short start;
	unsigned short len; /* set to (unsigned short)-1 for EOF */
};
typedef struct BPipe_tag BPipe;

int bpipe_init(BPipe *mypipe);
int bpipe_eof(BPipe *mypipe);
int bpipe_getc(BPipe *mypipe);
int bpipe_putc(int ch, BPipe *mypipe);
int bpipe_read(BPipe *mypipe, void *buffer, unsigned count);
int bpipe_write(BPipe *mypipe, void *buffer, unsigned count);
int bpipe_close(BPipe *mypipe);

#endif /* not BPIPE_H */
