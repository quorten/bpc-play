#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>

int rt_state = 0;
int rt_delay = 1;

int
main (int argc, char *argv[])
{
  mlockall (MCL_CURRENT | MCL_FUTURE);

  if (argc >= 2)
    rt_delay = atoi (argv[1]);

  while (1) {
    putchar ('\x08');
    switch (rt_state) {
    case 0: putchar ('|'); break;
    case 1: putchar ('/'); break;
    case 2: putchar ('-'); break;
    case 3: putchar ('\\'); break;
    }
    rt_state = (rt_state + 1) & (4 - 1);
    fflush (stdout);
    sleep (rt_delay);
  }
  return 0;
}
