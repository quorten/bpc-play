/* Serial version of sequence computation, cheap but simple.  Mainly
   useful for speed comparisons.  */

#include <stdio.h>
#include <stdlib.h>

#define BASE 1
#define COMBIN +

void next_n(int *data, unsigned n)
{
  int a = BASE;
  while (n > 0) {
    n--;
    *data++ = a;
    a = a COMBIN BASE;
  }
}

int main(int argc, char *argv[])
{
  unsigned n = (argc > 1) ? atoi(argv[1]) : 0;
  int *data = (int*)malloc(sizeof(int) * n);
  unsigned i;

  if (data == NULL)
    return 1;
  next_n(data, n);

#ifndef NOPRINT
  printf("%d", data[0]);
  for (i = 1; i < n; i++) {
    printf(" %d", data[i]);
  }
  puts("");
#endif

  return 0;
}
