#include <stdio.h>

int main()
{
	unsigned long i;
	FILE* fp;
	fp = fopen("fullblank.dsk", "wb");
	for (i = 0; i < (unsigned long)512 * 18 * 80 * 2; i++)
	{
		fputc(0xf6, fp);
	}

	fclose(fp);
	return 0;
}
