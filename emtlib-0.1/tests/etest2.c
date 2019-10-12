/* Another test module for expandable arrays.  */

#include <stdio.h>
#include <exparray.h>

EA_TYPE(int);

int main()
{
	int_array myvals;
	unsigned i;
	EA_INIT(int, myvals, 2);
	puts("Created an empty int array.");
	EA_APPEND(int, myvals, 5);
	puts("Added one value to the array in one way.");
	myvals.d[myvals.len] = 3;
	EA_SET_SIZE(int, myvals, myvals.len + 1);
	puts("Added another value in another way.");
	myvals.d[myvals.len] = 7;
	EA_SET_SIZE(int, myvals, myvals.len + 1);
	puts("Added one last value in one other way.");
	puts("Now let's look at the array:");
	for (i = 0; i < myvals.len; i++)
		printf("%i ", myvals.d[i]);
	puts("");
	puts("Now let's remove a value.");
	EA_REMOVE(int, myvals, 1);
	puts("Now let's look at the array:");
	for (i = 0; i < myvals.len; i++)
		printf("%i ", myvals.d[i]);
	puts("");
	puts("Alright, we're done.  Clean up.");
	EA_DESTROY(int, myvals);
	return 0;
}
