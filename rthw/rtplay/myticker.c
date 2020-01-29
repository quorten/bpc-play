#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int rt_state = 0;
long int rt_delay_sec = 1;
long int rt_delay_nsec = 0;

void
full_sleep (struct timespec *req)
{
  struct timespec rem1 = { 0, 0 }, rem2 = { 0, 0 };
  memcpy (&rem1, req, sizeof(struct timespec));
  while (1) {
    if (nanosleep (&rem1, &rem2) ==  -1) {
      if (nanosleep (&rem2, &rem1) == -1) {
	continue;
      }
    }
    break;
  }
}

int
main (int argc, char *argv[])
{
  if (argc >= 2)
    rt_delay_sec = atoi (argv[1]);
  if (argc >= 3)
    rt_delay_nsec = atoi (argv[2]);

  {
    struct timespec req = { rt_delay_sec, rt_delay_nsec };
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
      full_sleep (&req);
    }
  }
  return 0;
}
