/* Disk image transfers -- send/receive over COM, read/write to
   file, or read/write to raw disk drive.

   Input/Output modes:

   0: File
   1: Floppy disk
   2: Hard disk
   3: COM serial port
   4: LPT printer port, write-only, not yet tested!
   5: NUL
   6: ZERO
   7: CON

   N.B.: absread() and abswrite() only work for floppies.  biosdisk()
   is a bit harder to get working, but I appear to have got it working
   now.  Plus, it doesn't depend on MS-DOS, so you could literally
   create your own OS boot disk with this as a base if you wanted to.
   However, it doesn't work nicely for floppy disks when running under
   Windows, although hard disks have no issue.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bios.h>
#include <dos.h>

enum ImgIOMode_tag
{
	IMGIO_FILE = 0,
	IMGIO_FLOPPY, /* floppy disk */
	IMGIO_FDISK, /* "fixed disk" is a hard drive */
	IMGIO_COM, /* serial port */
	IMGIO_LPT, /* parallel port */
	IMGIO_NUL, /* NUL is same as /dev/null */
	IMGIO_ZERO, /* /dev/zero */
	IMGIO_CON, /* console */
	IMGIO_MEM, /* memory, system RAM */
	IMGIO_TFTP4, /* TFTP stream over UDPv4 socket */
	IMGIO_HTTP4, /* HTTP stream over TCPv4 socket */
	IMGIO_TFTP6, /* TFTP stream over UDPv6 socket */
	IMGIO_HTTP6, /* HTTP stream over TCPv6 socket */
	IMGIO_SZHEAD = 32, /* read/write a size header */
	IMGIO_MAGPKT = 64, /* read/write magic packet */
	/* Output only: Execute after copy to file or memory, or reboot
	   after copy to disk.  */
	/* Unless... as an extension, if we specify the target system
	   type, we can sometimes have a special instruction to run code
	   or reboot the system that we can send, i.e. if entering to a
	   monitor program on the destination.  Ah, writing a bootloader
	   is great guidance for learning what features a monitor program
	   must include.  Compute checksums in monitor program?  Not only
	   great for checking for manual entry operator error, but plugs
	   right into a bootloader.  */
	IMGIO_BOOT = 128,
	/* Input only, bi-directional communication link: Send a "ready"
	   magic packet on initialization.  */
	/* IMG_RDYPKT = 256 out of range!!!, */
	/* Decode/encode hexidecimal, read additional bootloader
	   instructions (flags) from magic packet, disk block offset to
	   start copying data input and output size, block padding,
	   checksum verification, duplex communication options for packet
	   retransmit, show bootloading status on screen, framebuffer
	   write, virtual keyboard via framebuffer and pointing device,
	   SLIP, PPP, DHCP, ... */
	/* Whoah, let's try to tone this thing down.  The virtual keyboard
	   and input from console?  That's a bit too much.  That's not a
	   bootloader, that's a front panel control switch or monitor
	   program.  Okay, okay, cut, carry on.  If a bootloader fails to
	   bring up one program, it brings up an alternate program, such
	   as a front panel control switch, monitor, high-level language,
	   and so on.  */
	/* What about bootloader menus?  You can have a menu, you can have
	   a prompt to type the name of a disk image to load...  Okay, I
	   think we need to scope this out.  Bootloader menus and status
	   screens, that is something of the more advanced and more modern
	   bootloaders that may be kind of more like a monitor program,
	   and they are something different.  Essentially, for menus, what
	   you're doing is you are loading (booting) a non-bootloader
	   program, that does its stuff, and then when you return, you
	   execute a bootloader.  Status screens?  Well, that is
	   definitely an advanced feature of bootloaders.

	   If it has a menu, it is a Boot Manager, not a Boot Loader.

	   Seriously, when I started out writing my disk imaging program,
	   it had very little in the way of reporting back status.  And
	   that is completely fine, it didn't need to be comprehensive in
	   that regard, that was the responsibility of other supplementary
	   programs.  What I would like to say is that if you have a
	   bootloader with status messages, what you technically have two
	   concurrent programs and the bootloader is simply passing
	   messages to the optional status program.

	   Okay, that's a great idea, I like it.  Here is the way I'll put
	   it.  The bootloader status is defined as internal state, but
	   such state that is stored in a well-defined structure.  Then
	   you have cooperative multitasking so that the status program
	   can gain access to the bootloader status variables, update its
	   displays, and do all that other stuff.  You can then choose
	   what status program you want to link against, or none at all.
	   Or, you can even do this.  Via function pointers, you can
	   bootload a new status program, then run that via the main
	   bootloading process.  */

	/* Important!  We need to keep a record of sectors in error from
	   our copies.  Either store up until the end or do an encoded
	   transfer copy.  */

	/* Yeah, you're right.  When you think about it, pre-emptive
	   multitasking would be the ideal for the status program, but
	   failing that, cooperative multitasking is your best bet.  It
	   works fine via subroutine calls in this paradigm because the
	   bootloader must be the first process to start, and the
	   bootloader terminates the status program when loading is
	   finished, just before transferring control to the new code.  */

	/* I really agree about the external status program thing... once
	   I started adding status code text strings in here, I looked at
	   the code and thought this is getting too complicated.  Also, we
	   do need a such thing as a "bootloader compiler" to generate a
	   stripped down bootloader including only the code necessary for
	   the selected variables.  It's definitely nice to have code that
	   can prpovide all options on hand for you, but for sure, I don't
	   need all options all the time.  I much better like the idea of
	   loading a new bootloader with additional logic if you need
	   something else, rather than defining one single mega
	   super-stage bootloader that you call back into for subsequent
	   boot stages and use a coded boot script to control.

	   Go modular.  Define abstract functions and use variables to
	   control how to link it all together.  Force function inlining
	   where there is only one caller.  Common stages for modules:
	   Configure, initialize, execute.  Module types: input, output,
	   convert.  */

	/* We do, of course, have compile-time and runtime options since
	   we do allow plugging in user interface front-ends to select
	   options beyond compile-time.  Okay... well the main point of
	   this is if you want a disk image copying utility, rather than a
	   bootloader.  But I digress.  That's the added layer of
	   complexity when you design a disk image utility to also double
	   as a bootloader.  */
};
typedef enum ImgIOMode_tag ImgIOMode;

struct CHSAddr_tag
{
	unsigned char drive;
	unsigned char head;
	unsigned short track;
	unsigned char sector;
};
typedef struct CHSAddr_tag CHSAddr;

/* Source or destination IPv4 address, port number, and filename.  */
struct IPv4Src_tag
{
	unsigned long ip;
	unsigned short port;
	char *filename;
};
typedef struct IPv4Src_tag IPv4Src;

/* Source or destination IPv6 address, port number, and filename.  */
struct IPv6Src_tag
{
	unsigned char ip[16];
	unsigned short port;
	char *filename;
};
typedef struct IPv6Src_tag IPv6Src;

/* Options in common with either a source or a destination.  */
struct SrcOpt_tag
{
	unsigned char mode;
	union
	{
		unsigned char num; /* Device ID number */
		char* filename;
		IPv4Src *ipv4src; /* IPv4 address, port, file */
		IPv6Src *ipv6src; /* IPv6 address, port, file */
	} id;
	unsigned long size;
	CHSAddr dsize;
	union
	{
		CHSAddr addr;
		FILE* fp;
	} work;
};

/* Multiply and divide by power-of-two by shifting.  This fast
   implementation divide by power of two by shifting is only accurate
   on unsigned integers.  */
#define MUL_256(val) ((val) << 8)
#define DIV_256(val) ((val) >> 8)
#define MUL_512(val) ((val) << 9)
#define DIV_512(val) ((val) >> 9)

/* Get the size in LBA sectors of a CHS size.  */
unsigned long compute_lba_size(CHSAddr *sizes)
{
	return (unsigned long)sizes->sector * sizes->track * sizes->head;
}

/* Zero a CHS address.  */
void zero_chs_addr(CHSAddr *addr)
{
	/* addr->drive = 0; */
	addr->head = 0;
	addr->track = 0;
	addr->sector = 0;
}

/* Convert an LBA sector address into a CHS address.  */
void compute_chs_addr(CHSAddr *addr, CHSAddr *sizes,
	unsigned long sector)
{
	if (addr == NULL || sizes == NULL)
		return;
	{
		unsigned char nsects = sizes->sector;
		unsigned long sects_per_head =
			(unsigned long)nsects * sizes->track;
		unsigned char cur_sect;
		unsigned short cur_track;
		unsigned char cur_head;
		/* addr->drive = 0; */
		addr->head = sector / sects_per_head;
		sector %= sects_per_head;
		addr->track = sector / (unsigned long)nsects;
		sector %= (unsigned long)nsects;
		addr->sector = sector;
	}
}

/* Use the biosdisk() call to get the size of a disk, then decode the
   results and place them in a CHSAddr structure.  */
int get_dsize(unsigned char disk_id, CHSAddr *sizes)
{
	unsigned char bd_buf[4]; /* BIOS disk buffer */
	if (sizes == NULL)
		return 1;
	{
		int result = biosdisk(8, disk_id, 0, 0, 0, 0, bd_buf);
		if (result != 0)
			return result;
		sizes->sector = bd_buf[0] & 0x3f;
		sizes->track = ((unsigned short)(bd_buf[0] & 0xc0) << 2) |
			bd_buf[1] + 1;
		sizes->drive = bd_buf[2];
		sizes->head = bd_buf[3] + 1;
	}
	return 0;
}

/* Read or write a number of 512-byte sectors.  `addr' is modified to
   point to the new curren disk address.  Return zero on success,
   nonzero on error.  */
int disk_bufop(unsigned char cmd,
			   CHSAddr *addr,
			   CHSAddr *sizes,
			   unsigned long nsects,
			   char* buffer)
{
	CHSAddr laddr = *addr;
	CHSAddr lsizes = *sizes;
	int result = 0;
	/* Walk through the CHS segments until the end.  */
	while (nsects > 0)
	{
		{
			unsigned short oper_sects;
			if (laddr.sector + nsects < lsizes.sector)
			{
				oper_sects = nsects;
				nsects = 0;
			}
			else
			{
				oper_sects = lsizes.sector - laddr.sector;
				nsects -= oper_sects;
			}
			result |= biosdisk(cmd, laddr.drive, laddr.head,
				laddr.track, 1 + laddr.sector, oper_sects, buffer);
			laddr.sector += oper_sects;
			buffer += MUL_512(oper_sects);
		}
		if (laddr.sector == lsizes.sector)
		{
			laddr.sector = 0;
			laddr.track++;
		}
		if (laddr.track == lsizes.track)
		{
			laddr.track = 0;
			laddr.head++;
		}
		if (laddr.head == lsizes.head)
			break; /* End of media */
	}
cleanup:
	*addr = laddr;
	if (nsects > 0)
	{
		if (result == 0)
			result = -1; /* clean EOF */
		else
			result |= 1;
	}
	return result;
}

int main(int argc, char* argv[])
{
	unsigned char in_mode;
	unsigned char out_mode;
	unsigned char in_id;
	unsigned char out_id;
	unsigned long in_size = (unsigned long)512 * 18 * 80 * 2;
	unsigned long out_size;
	CHSAddr in_daddr;
	CHSAddr out_daddr;
	CHSAddr in_dsize;
	CHSAddr out_dsize;
	char* in_filename;
	char* out_filename;
	unsigned long buffer_size = 512 * 9;
	unsigned char* buffer = NULL;
	FILE* in_fp = NULL;
	FILE* out_fp = NULL;
	unsigned long i;
	unsigned long j;
	int result;

	/* Parse arguments. */
	if (argc < 4)
	{
		fputs("Error: Invalid command line.\n", stderr);
		return 1;
	}
	{ /* If a non-zero input size is given, use it in place of the
	     default.  Note that this is still a "default" that is not
	     used in the case of disks, for example.  */
		long rd_in_size = atol(argv[1]);
		if (rd_in_size != 0)
			in_size = rd_in_size;
	}
	in_mode = argv[2][0] - '0';
	switch (in_mode)
	{
	case IMGIO_FILE:
		/* We will get the filename later.  */
		break;
	case IMGIO_FLOPPY:
	case IMGIO_COM:
		in_id = argv[2][1] - '0';
		break;
	case IMGIO_LPT:
		fputs("Error: LPT input not implemented.\n", stderr);
		return 1;
	case IMGIO_FDISK:
		in_id = argv[2][1] - '0' + 0x80;
		break;
	case IMGIO_NUL:
	case IMGIO_ZERO:
	case IMGIO_CON:
		break;
	default:
		fputs("Error: Unknown input mode.\n", stderr);
		return 1;
	}
	out_mode = argv[3][0] - '0';
	switch (out_mode)
	{
	case IMGIO_FILE:
		/* We will get the filename later.  */
		break;
	case IMGIO_FLOPPY:
	case IMGIO_COM:
	case IMGIO_LPT:
		out_id = argv[3][1] - '0';
		break;
	case IMGIO_FDISK:
		out_id = argv[3][1] - '0' + 0x80;
		break;
	case IMGIO_NUL:
	case IMGIO_ZERO:
	case IMGIO_CON:
		break;
	default:
		fputs("Error: Unknown output mode.\n", stderr);
		return 1;
	}
	i = 3;
	if (in_mode == IMGIO_FILE)
	{
		i++;
		if (argc <= i)
		{
			fputs("Error: Invalid command line.\n", stderr);
			return 1;
		}
		in_filename = argv[i];
		in_fp = fopen(in_filename, "rb");
		if (in_fp == NULL)
		{
			fputs("Error: Could not open input file.\n", stderr);
			return 1;
		}
		/* N.B.: Get file size does not work on streams.  */
		fseek(in_fp, 0, SEEK_END);
		in_size = ftell(in_fp);
		fseek(in_fp, 0, SEEK_SET);
	}
	if (out_mode == IMGIO_FILE)
	{
		i++;
		if (argc <= i)
		{
			fputs("Error: Invalid command line.\n", stderr);
			return 1;
		}
		out_filename = argv[i];
		out_fp = fopen(out_filename, "wb");
		if (out_fp == NULL)
		{
			fputs("Error: Could not open output file.\n", stderr);
			return 1;
		}
	}

	/* Initialize input devices and variables.  */
	if (in_mode == IMGIO_FLOPPY || in_mode == IMGIO_FDISK)
	{
		result = get_dsize(in_id, &in_dsize);
		if (result != 0)
		{
			fputs("Error: Failed to get input disk size.\n", stderr);
			return 1;
		}
		in_daddr.drive = in_id;
		zero_chs_addr(&in_daddr);
		in_size = MUL_512(compute_lba_size(&in_dsize));
	}
	/* Initialize output devices and variables.  */
	if (out_mode == IMGIO_FLOPPY || out_mode == IMGIO_FDISK)
	{
		result = get_dsize(out_id, &out_dsize);
		if (result != 0)
		{
			fputs("Error: Failed to get output disk size.\n", stderr);
			return 1;
		}
		out_daddr.drive = out_id;
		zero_chs_addr(&out_daddr);
		out_size = MUL_512(compute_lba_size(&out_dsize));
	}
	else if (out_mode == IMGIO_LPT)
		result = biosprint(1, 0, out_id);

	/* Allocate buffer memory.  */
	buffer = (unsigned char*)malloc(buffer_size);
	if (buffer == NULL)
	{
		fputs("Error: Could not allocate buffer.\n", stderr);
		return 1;
	}

	for (i = 0; i < in_size; i += buffer_size)
	{
		/* Bad sector flagging */
		j = 0;
		while (j < buffer_size)
		{
			buffer[j++] = 'B';
			buffer[j++] = 'A';
			buffer[j++] = 'D';
			buffer[j++] = ' ';
		}

		/* Actual read/write operation */
		switch (in_mode)
		{
		case IMGIO_FILE:
			fread(buffer, 512, DIV_512(buffer_size), in_fp);
			break;
		case IMGIO_FLOPPY:
			/* result = absread(in_id, DIV_512(buffer_size), DIV_512(i), buffer);
			if (result != 0)
				fprintf(stderr, "Error on disk input: %d\n", result);
			break; */
		case IMGIO_FDISK:
			result = disk_bufop(2, &in_daddr, &in_dsize, DIV_512(buffer_size), buffer);
			if (result != 0)
				fprintf(stderr, "Error on disk input: %d\n", result);
			break;
		case IMGIO_COM:
			for (j = 0; j < buffer_size; j++)
			{
				result = bioscom(2, 0, in_id);
				buffer[j] = (unsigned char)result;
			}
			break;
		case IMGIO_LPT:
			/* Only supported on PS/2 and newer.  */
			break;
		case IMGIO_NUL:
		case IMGIO_ZERO:
			memset(buffer, 0, buffer_size);
			break;
		case IMGIO_CON:
			for (j = 0; j < buffer_size; j++)
			{
				result = getchar();
				buffer[j] = (unsigned char)result;
			}
			break;
		}
		switch (out_mode)
		{
		case IMGIO_FILE:
			fwrite(buffer, 512, DIV_512(buffer_size), out_fp);
			break;
		case IMGIO_FLOPPY:
			/* result = abswrite(out_id, DIV_512(buffer_size), DIV_512(i), buffer);
			if (result != 0)
				fprintf(stderr, "Error on disk output: %d\n", result);
			break; */
		case IMGIO_FDISK:
			result = disk_bufop(3, &out_daddr, &out_dsize, DIV_512(buffer_size), buffer);
			if (result != 0)
				fprintf(stderr, "Error on disk output: %d\n", result);
			break;
		case IMGIO_COM:
			for (j = 0; j < buffer_size; j++)
			{
				result = bioscom(1, buffer[j], out_id);
			}
			break;
		case IMGIO_LPT:
			for (j = 0; j < buffer_size; j++)
			{
				result = biosprint(0, buffer[j], out_id);
			}
			break;
		case IMGIO_NUL:
		case IMGIO_ZERO:
			break;
		case IMGIO_CON:
			for (j = 0; j < buffer_size; j++)
				result = putchar(buffer[j]);
			break;
		}
	}
	fclose(in_fp);
	fclose(out_fp);
	free(buffer);
	return 0;
}
