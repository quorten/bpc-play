/* Find out how many boolean functions per number of inputs do not
   actually use that many inputs.

   NOTE: This code by definition cannot go very far without an
   arbitrary precision number library:  2^2^10 > 2^64.  */

#include <stdio.h>
#include <stdlib.h>

/* `digplace' is the place of the digit.  The least significant digit
   place is place zero.  `number' is the number to extract the digit
   at that place from.  */
int siggen(int digplace, int number)
{
  /* int stride; stride = (1 << digplace);
  return ((number / stride) % 2 > 0); */
  return (((number >> digplace) & 1) != 0);
}

int main(int argc, char *argv[])
{
  int maxclass, boolclass = 1;
  int verbose = 0;
  /* First choose a class of boolean functions that take a given
     number of inputs.  */
  if (argc > 2 && argv[1][0] == '-' && argv[1][1] == 'v')
    verbose = 1;
  if (argc > 1)
    {
      maxclass = atoi(argv[1]);
      if (maxclass >= 5)
	{
	  fprintf(stderr,
		  "tabgen: internal numerics cannot analyze that high.\n");
	  return 1;
	}
    }
  else
    maxclass = 3;
  while (boolclass <= maxclass)
    {
      int numfuncs;
      int i;
      int numdeads = 0;
      printf("Analyzing all boolean functions that take %i inputs...\n",
	     boolclass);
      /* Permutate over all possible boolean functions that take
	 `boolclass' number of inputs.  */
      numfuncs = (1 << (1 << boolclass));
      for (i = 0; i < numfuncs; i++)
	{
	  if (verbose)
	    printf("Testing function %#x.\n", i);
	  /* The current value of `i' represents the output values for
	     the current boolean function's truth table.  Now we need
	     to iterate over each input value.  */
	  int j;
	  for (j = 0; j < boolclass; j++)
	    {
	      if (verbose)
		printf("Testing variable position %i.\n", j);
	      /* Now we need another inner loop to iterate over all
		 the combinations of input values to complete this
		 truth table.  */
	      int k, numrows, deadvar = 1;
	      numrows = (1 << boolclass);
	      for (k = 0; k < numrows; k++)
		{
		  /* Look for all possible matching rows whose only
		     variable value that differs is the one currently
		     selected.  */
		  /* You can algorithmically find the matching pairs.
		     If you are at place zero, then evens and odds
		     match.  If you are at a larger place, then
		     matching pairs occur spaced at the block size.
		     The number of different pairs is equal to half
		     the total number of rows.  */
		  int blocksize, row1, row2, l;
		  blocksize = 1 << j;
		  row1 = k;
		  row2 = k + blocksize;
		  /* Now compare the output value at each row.  */
		  /* If the output values are different, then this
		     variable definitely affects the output value, so
		     you can break out and move onto the next
		     variable.  */
		  if (verbose)
		    {
		      printf("Testing");
		      for (l = boolclass; l > 0; l--)
			printf(" %i", siggen(l - 1, row1));
		      printf(" %i\n", siggen(row1, i));
		      printf("versus ");
		      for (l = boolclass; l > 0; l--)
			printf(" %i", siggen(l - 1, row2));
		      printf(" %i\n", siggen(row2, i));
		    }
		  if (siggen(row1, i) != siggen(row2, i))
		    {
		      deadvar = 0;
		      break;
		    }
		  if (blocksize == 1)
		    k++; /* Special case for non-interleaved pairs.  */
		  else if (k + blocksize + 1 >= numrows)
		    break;
		}
	      if (deadvar)
		{
		  printf("One phony function found: %#x (%i).\n", i, i);
		  numdeads++;
		  /* You only need to find one dead variable to know
		     that a function doesn't use all of its inputs.
		     However, you can do further analysis if you want
		     to know how many dead variables a certain
		     function has.  */
		  break;
		}
	    }
	}
      /* Summarize this cycle's computations.  */
      printf("Total of %i phony functions found in this cycle.\n", numdeads);
      numdeads = 0;
      boolclass++;
    }
  return 0;
}
