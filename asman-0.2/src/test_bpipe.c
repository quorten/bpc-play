/* Test simple boot pipe implementation.

  2019

How will we do this?  We collect some data on input, write it through
the pipe, read it through the pipe, in a loop.  We compute a simple
XOR checksum on both sides and verifies that it matches.
Then we also compare the two buffers to make sure they match.

And we randomize the amount of bytes read and written in one call.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bpipe.h"

int main()
{
	unsigned char buf_in[8192];
	unsigned char in_xor_cksum[4] = { 0, 0, 0, 0 };
	unsigned char buf_out[8192];
	unsigned char out_xor_cksum[4] = { 0, 0, 0, 0 };
	unsigned char xor_pos;
	unsigned i, j;

	BPipe mypipe;

	xor_pos = 0;
	for (i = 0; i < 8192; i++)
	{
		unsigned char val = rand() & (256 - 1);
		buf_in[i] = val;
		in_xor_cksum[xor_pos++] ^= val;
		xor_pos &= 4 - 1;
	}

	/* First test simple case of `getc()'/`putc()'.  */

	bpipe_init (&mypipe);

	xor_pos = 0;
	i = 0, j = 0;
	while (!bpipe_eof(&mypipe))
	{
		int ch;
		while (i < 8192 && bpipe_putc(buf_in[i], &mypipe) == buf_in[i])
			i++;
		/* N.B. We repeatedly try to close the pipe until the buffer
		   is empty.  */
		if (i >= 8192)
			bpipe_close(&mypipe);
		if (bpipe_eof(&mypipe))
			break;
		while ((ch = bpipe_getc(&mypipe)) != -1 && ch != -2)
		{
			buf_out[j++] = (unsigned char)ch;
			out_xor_cksum[xor_pos++] ^= (unsigned char)ch;
			xor_pos &= 4 - 1;
		}
	}

	/* Now verify the data is equivalent.  */
	if (memcmp(in_xor_cksum, out_xor_cksum, 4) != 0)
	{
		fputs("XOR checksums do not match!\n", stderr);
		return 1;
	}
	else if (memcmp(buf_in, buf_out, 8192) != 0)
	{
		fputs("Input and output buffers do not match!", stderr);
		return 1;
	}

	/* TODO: Now test read/write in larger random-sized blocks.  */

	return 0;
}
