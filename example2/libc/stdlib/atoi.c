/* A simple `atoi()' function defined only for ASCII systems.  */
/* See the COPYING file for license details.  */

#define MAX_DIGITS (10)

int
atoi (const char *str)
{
  int read_num = 0;
  unsigned char read_num_mult = 0;
  unsigned char neg = 0;
  char c;
  if (*str == '-') {
    neg = 1;
    str++;
  }
  while ((c = *str++) >= '0' && c <= '9') {
    /* read_num *= 10; */
    read_num = (read_num << 3) + (read_num << 1);
    read_num += c - '0';
    read_num_mult++;
    if (read_num_mult >= MAX_DIGITS)
      return 0;
  }
  if (neg)
    read_num = -read_num;
  return read_num;
}
