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
	if (/* in_mode == IMGIO_FLOPPY || */ in_mode == IMGIO_FDISK)
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
	if (/* out_mode == IMGIO_FLOPPY || */ out_mode == IMGIO_FDISK)
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
		unsigned long read_size = buffer_size;
		/* Bad sector flagging */
		j = 0;
		while (j < buffer_size)
		{
			buffer[j++] = 'B';
			buffer[j++] = 'A';
			buffer[j++] = 'D';
			buffer[j++] = ' ';
		}
		/* Check if we have a non-divisible block size at the
		   end.  */
		if (i + buffer_size > in_size)
			read_size = in_size - i;

		/* Actual read/write operation */
		switch (in_mode)
		{
		case IMGIO_FILE:
			fread(buffer, 512, DIV_512(read_size), in_fp);
			break;
		case IMGIO_FLOPPY:
			result = absread(in_id, DIV_512(read_size), DIV_512(i), buffer);
			if (result != 0)
				fprintf(stderr, "Error on disk input: %d\n", result);
			break;
		case IMGIO_FDISK:
			result = disk_bufop(2, &in_daddr, &in_dsize, DIV_512(read_size), buffer);
			if (result != 0)
				fprintf(stderr, "Error on disk input: %d\n", result);
			break;
		case IMGIO_COM:
			for (j = 0; j < read_size; j++)
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
			memset(buffer, 0, read_size);
			break;
		case IMGIO_CON:
			for (j = 0; j < read_size; j++)
			{
				result = getchar();
				buffer[j] = (unsigned char)result;
			}
			break;
		}
		switch (out_mode)
		{
		case IMGIO_FILE:
			fwrite(buffer, 512, DIV_512(read_size), out_fp);
			break;
		case IMGIO_FLOPPY:
			result = abswrite(out_id, DIV_512(read_size), DIV_512(i), buffer);
			if (result != 0)
				fprintf(stderr, "Error on disk output: %d\n", result);
			break;
		case IMGIO_FDISK:
			result = disk_bufop(3, &out_daddr, &out_dsize, DIV_512(read_size), buffer);
			if (result != 0)
				fprintf(stderr, "Error on disk output: %d\n", result);
			break;
		case IMGIO_COM:
			for (j = 0; j < read_size; j++)
			{
				result = bioscom(1, buffer[j], out_id);
			}
			break;
		case IMGIO_LPT:
			for (j = 0; j < read_size; j++)
			{
				result = biosprint(0, buffer[j], out_id);
			}
			break;
		case IMGIO_NUL:
		case IMGIO_ZERO:
			break;
		case IMGIO_CON:
			for (j = 0; j < read_size; j++)
				result = putchar(buffer[j]);
			break;
		}
	}
	fclose(in_fp);
	fclose(out_fp);
	free(buffer);
	return 0;
}
