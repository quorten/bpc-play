/* lnmux.c -- Multiplex the lines of the input files.

Copyright (C) 2013 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_SIZE 4096

int main(int argc, char *argv[])
{
	FILE **inputs;
	unsigned num_inputs;
	char *fgets_ret = (char*)(!NULL);

	/* Parse the command line.  */
	if (argc == 1)
	{
		printf("Usage: %s FILES... > MUX\n", argv[0]);
		puts("Multiplex the lines of each file to standard output.\n"
"Multiplexing stops after a failure to read the next line from one file.");
		return 0;
	}

	/* Allocate memory for file pointers.  */
	num_inputs = argc - 1;
	inputs = (FILE**)malloc(sizeof(FILE*) * num_inputs);
	if (inputs == NULL)
	{
		fprintf(stderr, "%s: Could not allocate any "
				"memory for file pointers!\n", argv[0]);
		return 1;
	}

	{ /* Open all the input files.  */
		unsigned i;
		for (i = 0; i < num_inputs; i++)
		{
			inputs[i] = fopen(argv[i+1], "r");
			if (inputs[i] == NULL)
			{
				unsigned j;
				fprintf(stderr, "%s: ", argv[0]);
				perror(argv[i+1]);
				fprintf(stderr, "Type `%s' with no arguments for help.\n",
						argv[0]);
				for (j = 0; i != 0 && j < i - 1; j++)
					fclose(inputs[j]);
				free(inputs);
				return 1;
			}
		}
	}

	do /* Do the multiplexing.  */
	{
		unsigned i;
		for (i = 0; fgets_ret != NULL && i < num_inputs; i++)
		{
			char buffer[BUFF_SIZE];
			do {
				fgets_ret = fgets(buffer, BUFF_SIZE, inputs[i]);
				if (fgets_ret == NULL)
					break;
				fputs(buffer, stdout);
			} while (buffer[strlen(buffer)-1] != '\n');
		}
	} while (fgets_ret != NULL);

	{ /* Cleanup */
		unsigned i;
		for (i = 0; i < num_inputs; i++)
			fclose(inputs[i]);
		free(inputs);
	}
	return 0;
}
