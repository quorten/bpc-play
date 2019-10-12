/* See the COPYING file for license details.  */
#ifndef STDLIB_H
#define STDLIB_H

#include "stddef.h"

#define NULL            0
#define RAND_MAX        2147483647

void exit (int status);
void abort (void);
int atoi (const char *str);
int rand (void);
void srand (unsigned int seed);

#endif
