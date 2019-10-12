/* Test module for expandable arrays.  */

#include <stdio.h>
#include <stdlib.h>

#include <exparray.h>

EA_TYPE(int);

int main()
{
	int_array myArray;
	unsigned i;

	puts("Testing expandable arrays for ints.");
	EA_INIT(int, myArray, 16);
	srand(20);

	for (i = 0; i < 25; i++)
	{
		myArray.d[i] = rand();
		EA_ADD(int, myArray);
	}

	puts("Here are the contents of the array: ");

	for (i = 0; i < myArray.len; i++)
		printf("%i \n", myArray.d[i]);

	/* Cleanup */
	free(myArray.d);

	return 0;
}
