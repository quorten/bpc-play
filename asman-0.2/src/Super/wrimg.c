/* Write a floppy disk image */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

int main()
{
	char* myBuffer;
	FILE* fp;
	long i;

	myBuffer = malloc(512 * 18);
	if (myBuffer == NULL)
		return 1;
	fp = fopen("image.dsk", "rb");
	if (fp == NULL)
		return 2;
	for (i = 0; i < 18 * 80 * 2; i += 18)
	{
		fread(myBuffer, 18, 512, fp);
		abswrite(0, 18, i, myBuffer);
	}
	fclose(fp);
	free(myBuffer);
	return 0;
}
