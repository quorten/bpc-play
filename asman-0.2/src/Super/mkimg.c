/* Write a disk image to a file */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

int main()
{
	char* myBuffer;
	FILE* fp;
	long i;
        long j;
	int result;

	myBuffer = malloc(512 * 18);
	if (myBuffer == NULL)
		return 1;
	fp = fopen("image.dsk", "wb");
	if (fp == NULL)
		return 2;
	for (i = 0; i < 18 * 80 * 2; i += 18)
	{
                /* Bad sector flagging */
                j = 0;
                while (j < 512 * 18)
                {
                        myBuffer[j++] = 'B';
                        myBuffer[j++] = 'A';
                        myBuffer[j++] = 'D';
                        myBuffer[j++] = ' ';
                }

                /* Actual read operation */
		absread(0, 18, i, myBuffer);
		fwrite(myBuffer, 18, 512, fp);
	}
	fclose(fp);
	free(myBuffer);
	return 0;
}
