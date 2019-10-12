/* bmon.c -- A simple bootstrap "monitor" program, like the Apple II
   monitor, but stripped down to be as small as possible, while still
   serving its primary purpose.

   2019 */

/*

In case computer moderners may have forgotten.  Why do we start out by
typing machine code in hex byte codes for type-in assembly language
programs?  In general, and especially on early 8-bit systems, the hex
byte codes are shorter to type-in than the equivalent assembly
language.  Therefore, it is faster to type-in programs in machine code
than assembly language, assuming your program has already been
designed in advance.

Please note that a good trick that we could use to reduce the amount
of generated code is to avoid using Boolean logical operators where
there is otherwise complex short-circuit evaluation.  The simple case
of short circuit evaluation (all logical AND or all logical OR) is
pretty easy to generate efficient and compact code.

Unfortunately, as it turns out, though XOR checksumming is a useful
feature, it adds A LOT of machine code for this otherwise relatively
tiny program.

Okay, but here, we have a workable strategy.  First compile a
bare-minimum version without XOR checksumming, but uses subroutines
for read and write memory.  Type that in, and boot it up.  Next,
type-in the improved read and write with XOR checksumming, and rewrite
the existing monitor code in RAM to use that.  Now you're ready to
load your truly large type-in program.

Useful test on i386 Linux, display "Heck.\n" on standard output:

Disassembly of section .text:

00000000 <.text>:
   0:   53                      push   %ebx
   1:   52                      push   %edx
   2:   68 2e 0a 00 00          push   $0xa2e
   7:   68 48 65 63 6b          push   $0x6b636548
   c:   b8 04 00 00 00          mov    $0x4,%eax
  11:   bb 00 00 00 00          mov    $0x0,%ebx
  16:   89 e1                   mov    %esp,%ecx
  18:   ba 06 00 00 00          mov    $0x6,%edx
  1d:   cd 80                   int    $0x80
  1f:   58                      pop    %eax
  20:   58                      pop    %eax
  21:   5a                      pop    %edx
  22:   5b                      pop    %ebx
  23:   c3                      ret    

Example usage of the bootstrap monitor, type this in:

0C00- 53 52 68 2E 0A 00 00 68 X3114
0C08- 48 65 63 6B B8 04 00 00 X930A
0C10- 00 BB 00 00 00 00 89 E1 X895A
0C18- BA 06 00 00 00 CD 80 58 X3A93
0C20- 58 5A 5B C3 00 00 00 00 X0399
0C00.0C27
0C00G

Other noteworthy tricks:

* Type a memory address and ENTER to dump one line of memory.

* Press ENTER repeatedly to dump the next line of memory.

* Type "." (dot) ADDR and ENTER to dump memory from the last address
  up to the given address.

* Type "G" to execute at the last address.

* You can omit the address and type "-" when writing memory to
  continue from the last address.

* ":" (colon) is also supported on entry for compatibility with Apple
  II binary listings.

* The XOR checksum at the end (example X3114) is optional when writing
  memory.

*/

#define ishex(ch) (((ch >= '0') && (ch <= '9')) || \
				   ((ch >= 'A') && (ch <= 'F')) || \
				   ((ch >= 'a') && (ch <= 'f')))
#define EOF (-1)
#define STDOUT_FILENO 0
void exit(int status);
int getchar(void);
int putchar(int ch);
long write_all(int fd, const void *buf, unsigned long count);

#define USE_XOR_CK
#define XOR_CK_LEN 2

/* 64 KiB of memory for a 16-bit address space, shared with the
   `.text' executable code section, which is marked as writable.  */
extern unsigned char memory[64*1024];
/* This is the trick we use.  memspace goes in `.bss' which comes
   right after `.text', which is the acutal padding for the variable,
   but we define `memory' to point to the start of this program's code
   so that it can be read and modified.  */
unsigned char memspace[64*1024];
unsigned short last_addr;
#ifdef USE_XOR_CK
unsigned short error_count = 0;
#endif

void putprompt(void);
void puthex(unsigned char len, unsigned short data);
unsigned short parsehex(unsigned char len, char *data);
unsigned short gethex(unsigned char maxlen, int *rch);
void dumphex(unsigned char *memory,
			 unsigned short addr, unsigned short end_addr,
			 unsigned char one_line);
void writehex(unsigned char *memory, int *rch);

void main(void)
{
	int ch;
	putprompt();
	ch = getchar();
	while (ch != EOF)
	{
		if (ch == '\n')
		{
			/* Display one line of hex dump.  */
			unsigned short addr = last_addr;
			unsigned short end_addr = addr + 8;
			dumphex(memory, addr, end_addr, 1);
		}
		else if (ishex(ch))
		{
			/* Read an address.  */
			last_addr = gethex(4, &ch);
			/* Skip standard actions at end of loop, we may have
			   additional commands to read.  */
			continue;
		}
		else if (ch == '.')
		{
			/* Read memory range.  */
			unsigned short end_addr;
			ch = getchar();
			if (ch == EOF)
				break;
			end_addr = gethex(4, &ch);
			dumphex(memory, last_addr, end_addr, 0);
		}
		else if (ch == '-' || ch == ':')
		{
			/* Write bytes to address.  */
			writehex(memory, &ch);
			if (ch == EOF)
				break;
		}
		else if (ch == 'G' || ch == 'g')
		{
			/* Ignore characters until end of newline.  */
			while ((ch = getchar()) != EOF && ch != '\n');

			/* Execute!  */
			((void (*)(void))&memory[last_addr])();
		}
		else if (ch == ' ')
		{
			/* Horizontal whitespace, ignore.  */
			ch = getchar();
			continue;
		}
		else
		{
			putchar('\a'); putchar('E'); putchar('\n');
		}
		putprompt();
		ch = getchar();
	}
	putchar('\n');
	exit (0);
}

/* N.B. We use a special function `putprompt()' to ensure that we can
   write the asterisk without line output buffering.  */
void putprompt(void)
{
	char buf = '*';
	write_all(STDOUT_FILENO, &buf, 1);
}

/* `len' is length in hex chars, should be either 2 (byte) or 4
   (word).

   N.B. Shifting is expensive on early 8-bit processors because you
   can only shift one bit at a time, so we try to minimize that
   here.  */
void puthex(unsigned char len, unsigned short data)
{
	char buf[4];
	unsigned char i = len;
	while (i > 0)
	{
		unsigned char val;
		i--;
		val = data & 0x0f;
		data >>= 4;
		if (val < 0xa)
			val += '0';
		else
			val += 'A' - 0xa;
		buf[i] = val;
	}
	while (i < len)
	{
		putchar(buf[i++]);
	}
}

unsigned short parsehex(unsigned char len, char *data)
{
	unsigned short result = 0;
	unsigned char i = 0;
	while (i < len)
	{
		unsigned char val;
		val = data[i];
		if (val >= 'a')
			val -= 'a' - 0xa;
		else if (val >= 'A')
			val -= 'A' - 0xa;
		else
			val -= '0';
		val &= 0x0f;
		result <<= 4;
		result |= val;
		i++;
	}
	return result;
}

/* TODO: Every time after calling gethex(), check if we are stuck on
   an invalid non-hex char.  */
unsigned short gethex(unsigned char maxlen, int *rch)
{
	unsigned short result;
	int ch;
	char rdbuf[4];
	unsigned rdbuf_len = 0;
	if (maxlen > 4) /* programmer error, i.e. assert() failure */
		return 0;
	ch = *rch;
	while (ch != EOF && rdbuf_len < maxlen && ishex(ch))
	{
		rdbuf[rdbuf_len++] = (char)ch;
		ch = getchar();
	}
	*rch = ch;
	result = parsehex(rdbuf_len, rdbuf);
	return result;
}

void dumphex(unsigned char *memory,
			 unsigned short addr, unsigned short end_addr,
			 unsigned char one_line)
{
#ifdef USE_XOR_CK
	unsigned char xor_cksum[XOR_CK_LEN] = { 0, 0/*, 0, 0*/ };
	unsigned char xor_pos = 0;
#endif
	puthex(4, addr); putchar('-');
	/* TODO FIXME: I trid to fold the last iteration into here to
	   reduce code, but that introduces a bug that does not properly
	   handle "0000.ffff".  Fix this.  */
	/* N.B. If end_addr < addr, we print one byte at addr,
	   similar to the Apple II monitor.  */
	do	
{
		unsigned char val = memory[addr++];
#ifdef USE_XOR_CK
		xor_cksum[xor_pos++] ^= val;
		xor_pos &= XOR_CK_LEN - 1;
#endif
		putchar(' ');
		puthex(2, val);
		if ((addr & 0x07) == 0x00)
		{
#ifdef USE_XOR_CK
			/* Print XOR checksum.  */
			putchar(' ');
			putchar('X');
			puthex(4, ((unsigned short)xor_cksum[0] << 8) | xor_cksum[1]);
			/* puthex(4, ((unsigned short)xor_cksum[2] << 8) | xor_cksum[3]); */
			xor_cksum[0] = 0; xor_cksum[1] = 0;
			/* xor_cksum[2] = 0; xor_cksum[3] = 0; */
#endif
			if (one_line)
				break;
			if (addr <= end_addr)
			{
				putchar('\n');
				puthex(4, addr); putchar('-');
			}
		}
	} while (addr <= end_addr);
	putchar('\n');
	last_addr = addr;
}

void writehex(unsigned char *memory, int *rch)
{
	int ch;
	unsigned short addr = last_addr;
#ifdef USE_XOR_CK
	unsigned char xor_cksum[XOR_CK_LEN] = { 0, 0/*, 0, 0*/ };
	unsigned char xor_pos = 0;
#endif
	ch = getchar();
	if (ch == EOF)
		goto cleanup;
	do
	{
		unsigned char val;
		while (ch == ' ')
			ch = getchar();
		if (ch == EOF || ch == '\n' || ch == 'X' || ch == 'x')
			break;
		val = (unsigned char)gethex(2, &ch);
#ifdef USE_XOR_CK
		xor_cksum[xor_pos++] ^= val;
		xor_pos &= XOR_CK_LEN - 1;
#endif
		memory[addr++] = val;
	} while (ch != '\n' && ch != 'X' && ch != 'x');
#ifdef USE_XOR_CK
	if (ch == 'X' || ch == 'x')
	{
		/* Read and validate XOR checksum.  */
		unsigned char rd_cksum[XOR_CK_LEN] = { 0, 0/*, 0, 0*/ };
		ch = getchar();
		if (ch == EOF)
			goto cleanup;
		rd_cksum[0] = (unsigned char)gethex(2, &ch);
		rd_cksum[1] = (unsigned char)gethex(2, &ch);
		/* rd_cksum[2] = (unsigned char)gethex(2, &ch);
		   rd_cksum[3] = (unsigned char)gethex(2, &ch); */
		if (xor_cksum[0] != rd_cksum[0] ||
			xor_cksum[1] != rd_cksum[1] /* ||
			xor_cksum[2] != rd_cksum[2] ||
			xor_cksum[3] != rd_cksum[3] */)
		{
			putchar('\a'); putchar('E');
			/* With our current checksumming algorithm, after
			   128 detected errors, it is pretty much
			   guaranteed that there may be one undetected
			   error.  */
			if (error_count >= 128)
			{ putchar('\a'); putchar('!'); }
			else
				error_count++;
			putchar('\n');
			/* Rewind to `last_addr' on error.  */
			addr = last_addr;
		}
	}
#endif
	last_addr = addr;
cleanup:
	*rch = ch;
}
