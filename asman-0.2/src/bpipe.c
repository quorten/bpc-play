/* bpipe.c -- Very simple pipe implementation, suitable for
   implementation in early boot environments.

   2019  */

#include <string.h>

#include "bpipe.h"

int bpipe_init(BPipe *mypipe)
{
	if (mypipe == NULL)
		return -1;
	mypipe->start = 0;
	mypipe->len = 0;
	return 0;
}

int bpipe_eof(BPipe *mypipe)
{
	return mypipe->len == (unsigned short)-1;
}

int bpipe_getc(BPipe *mypipe)
{
	unsigned char ch;
	if (mypipe->len == 0)
		return -2; /* EWOULDBLOCK */
	else if (mypipe->len == (unsigned short)-1)
		return -1; /* EOF */
	ch = mypipe->buf[mypipe->start];
	mypipe->start = (mypipe->start + 1) & (BPIPE_BUFSIZE - 1);
	mypipe->len--;
	return ch;
}

int bpipe_putc(int ch, BPipe *mypipe)
{
	unsigned short end;
	if (mypipe->len >= BPIPE_BUFSIZE)
		return -2; /* EWOULDBLOCK */
	else if (mypipe->len == (unsigned short)-1)
		return -1; /* EOF */
	end = (mypipe->start + mypipe->len) & (BPIPE_BUFSIZE - 1);
	mypipe->buf[end] = (unsigned char)ch;
	mypipe->len++;
	return (int) ((unsigned char)ch);
}

int bpipe_read(BPipe *mypipe, void *buffer, unsigned count)
{
	unsigned char *buf_ptr = buffer;
	/* Copy the data in up to two block segments.  */
	unsigned short s1_start = mypipe->start;
	unsigned short s1_len = BPIPE_BUFSIZE - s1_start;
	/* unsigned short s2_start = 0; */
	unsigned short s2_len = (unsigned short)-1;
	if (mypipe->len == 0)
		return -2; /* EWOULDBLOCK */
	else if (mypipe->len == (unsigned short)-1)
		return -1; /* EOF */
	else if (count > mypipe->len)
		count = mypipe->len;
	if (count <= s1_len)
		s1_len = count;
	else
		s2_len = count - s1_len;
	memcpy (buf_ptr, &mypipe->buf[s1_start], s1_len);
	buf_ptr += s1_len;
	if (s2_len != (unsigned short)-1)
		memcpy (buf_ptr, &mypipe->buf[0], s2_len);

	mypipe->start = (mypipe->start + count) & (BPIPE_BUFSIZE - 1);
	mypipe->len -= count;
	return count;
}

int bpipe_write(BPipe *mypipe, void *buffer, unsigned count)
{
	unsigned char *buf_ptr = buffer;
	/* Copy the data in up to two block segments.  */
	unsigned short s1_start = mypipe->start + mypipe->len;
	unsigned short s1_len = BPIPE_BUFSIZE - s1_start;
	/* unsigned short s2_start = 0; */
	unsigned short s2_len = (unsigned short)-1;
	unsigned short free_len = BPIPE_BUFSIZE - mypipe->len;
	if (free_len == 0)
		return -2; /* EWOULDBLOCK */
	else if (mypipe->len == (unsigned short)-1)
		return -1; /* EOF */
	else if (count > free_len)
		count = free_len;
	if (count <= s1_len)
		s1_len = count;
	else
		s2_len = count - s1_len;
	memcpy (&mypipe->buf[s1_start], buf_ptr, s1_len);
	buf_ptr += s1_len;
	if (s2_len != (unsigned short)-1)
		memcpy (&mypipe->buf[0], buf_ptr, s2_len);

	mypipe->len += count;
	return count;
}

int bpipe_close(BPipe *mypipe)
{
	if (mypipe->len == (unsigned short)-1)
		return -1; /* EOF, already closed */
	else if (mypipe->len != 0)
		return -2; /* EWOULDBLOCK */
	mypipe->len = (unsigned short)-1;
	return 0;
}
