/* Extract a raw disk image from a DiskCopy 4.2 disk image.  If the
   third argument `t' is given, then interleave file tags in the
   extracted raw disk image as if it is a image of "524-byte"
   sectors.  */

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
	int write_tags = 0;
	DC42Head head;
	char *input_name = argv[1];
	char *output_name = argv[2];
	unsigned name_len;
	FILE *in_fp;
	FILE *out_fp;
	unsigned i;

	if (argc != 3 && argc != 4)
		return 1;
	if (argc == 4 && argv[3][0] == 't')
		write_tags = 1;

	/* Read the input.  */
	in_fp = fopen(input_name, "rb");
	if (in_fp == NULL)
		return 1;

	fread(&head, sizeof(DC42Head), 1, in_fp);
	fread(image_data, 512 * 1600, 1, in_fp);
	fread(tag_data, 12 * 1600, 1, in_fp);

	fclose(in_fp);

	/* Convert to host endian.  */
	head.DataSize = ntohl(head.DataSize);
	head.TagSize = ntohl(head.TagSize);
	head.DataCksum = ntohl(head.DataCksum);
	head.TagCksum = ntohl(head.TagCksum);
	head.PrivateWord = ntohs(head.PrivateWord);

	/* Write the reformatted output.  */
	out_fp = fopen(output_name, "wb");
	if (out_fp == NULL)
		return 1;

	for (i = 0; i < 1600; i++)
	{
		fwrite(image_data + 512 * i, 512, 1, out_fp);
		if (write_tags)
			fwrite(tag_data + 12 * i, 12, 1, out_fp);
	}

	fclose(out_fp);

	return 0;
}
