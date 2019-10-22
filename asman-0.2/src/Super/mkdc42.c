/* Convert a Macintosh 800K "524-byte sector" disk image to a DiskCopy
   4.2 disk image.  That is, each sector is 512 data bytes followed by
   the 12 file tags bytes.

   20191020/https://wiki.68kmla.org/DiskCopy_4.2_format_specification
*/

#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>

struct DC42Head_tag
{
	unsigned char PNamLen;
	char Name[63];
	unsigned int DataSize;
	unsigned int TagSize;
	unsigned int DataCksum;
	unsigned int TagCksum;
	unsigned char DiskEncode;
	unsigned char FormatByte;
	unsigned short PrivateWord;
} __attribute__((packed));
typedef struct DC42Head_tag DC42Head;

unsigned char image_data[512*1600];
unsigned char tag_data[12*1600];

int main(int argc, char *argv[])
{
	DC42Head head;
	char *input_name = argv[1];
	char *output_name = argv[2];
	unsigned name_len;
	FILE *in_fp;
	FILE *out_fp;
	unsigned i;

	if (argc != 3)
		return 1;

	/* Read and reformat the input.  */
	in_fp = fopen(input_name, "rb");
	if (in_fp == NULL)
		return 1;

	name_len = strlen(input_name);
	if (name_len > 63)
		name_len = 63;
	head.PNamLen = name_len;
	memset(head.Name, 0, 63);
	strncpy(head.Name, input_name, name_len);
	head.DataSize = 512 * 1600;
	head.TagSize = 12 * 1600;
	head.DataCksum = 0;
	head.TagCksum = 0;

	for (i = 0; i < 1600; i++)
	{
		unsigned j;
		unsigned short *cksum_ptr;
		fread(image_data + 512 * i, 512, 1, in_fp);
		fread(tag_data + 12 * i, 12, 1, in_fp);
		cksum_ptr = (unsigned short*)(image_data + 512 * i);
		for (j = 0; j < 256; j++)
		{
			head.DataCksum += ntohs(*cksum_ptr);
			/* Rotate right.  */
			head.DataCksum =
				(head.DataCksum >> 1) | ((head.DataCksum & 1) << 31);
			cksum_ptr++;
		}
		if (i == 0)
			continue; /* Bug-compatigble checksum computation.  */
		cksum_ptr = (unsigned short*)(tag_data + 12 * i);
		for (j = 0; j < 6; j++)
		{
			head.TagCksum += ntohs(*cksum_ptr);
			/* Rotate right.  */
			head.TagCksum =
				(head.TagCksum >> 1) | ((head.TagCksum & 1) << 31);
			cksum_ptr++;
		}
	}
	fclose(in_fp);

	head.DiskEncode = 1; /* 01 = GCR CLV dsdd (800k) */
	head.FormatByte = 0x22; /* $22 = Disk formatted as Mac 800k */
	head.PrivateWord = 0x0100;

	/* Convert to big endian.  */
	head.DataSize = htonl(head.DataSize);
	head.TagSize = htonl(head.TagSize);
	head.DataCksum = htonl(head.DataCksum);
	head.TagCksum = htonl(head.TagCksum);
	head.PrivateWord = htons(head.PrivateWord);

	/* Now write the output.  */
	out_fp = fopen(output_name, "wb");
	if (out_fp == NULL)
		return 1;

	fwrite(&head, sizeof(DC42Head), 1, out_fp);
	fwrite(image_data, 512 * 1600, 1, out_fp);
	fwrite(tag_data, 12 * 1600, 1, out_fp);

	fclose(out_fp);

	return 0;
}
