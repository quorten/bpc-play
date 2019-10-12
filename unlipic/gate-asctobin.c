/* Very simple compiler that translates an ASCII gate array listing to
   the binary format required for the simulator.  Reads from standard
   input, writes to standard output.  */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "bool.h"

#define SYNTAX_ERROR(msg) ({ \
    fprintf(stderr, "Syntax error on line %u: %s\n", line_num, msg); \
    return 1; \
  })
#ifdef O_TEXT
#define WRITE_BIT(bit) ((bit) ? putchar('1') : putchar('0'))
#else
#define WRITE_BIT(bit) ({ \
    bytebuf |= (bit) << bufsize++; \
    if (bufsize == 8) { \
      putchar((unsigned int)bytebuf); \
      bytebuf = bufsize = 0; \
    } \
  })
#endif
#define OLD_WRITE_OPERAND(bitwidth, operand) ({ \
    static bool wrote_first_insn = false; \
    unsigned j = 0; \
    if (wrote_first_insn) \
      WRITE_BIT(0); /* Space indicator */ \
    else wrote_first_insn = true; \
    for (j = 0; j < bitwidth; j++) { \
      WRITE_BIT(1); /* Mark indicator */ \
      WRITE_BIT(((operand) >> j) & 1); \
    } \
 })
#define WRITE_OPERAND(operand) ({ \
    unsigned j = 0; \
    for (j = 0; j < sizeof(unsigned); j++) \
      putchar((operand >> (j * 8)) & 0xff); \
 })

#define PT_NAND 0
#define PT_OR 1

int
main (int argc, char *argv[])
{
  unsigned char process_type = PT_NAND;
  int c;
  unsigned line_num = 1;
  /* Bits buffered into a byte not written to output yet.  */
  unsigned char bytebuf = 0;
  unsigned char bufsize = 0;

  if (argc == 2) {
    if (strcmp (argv[1], "NAND") == 0)
      process_type = PT_NAND;
    else if (strcmp (argv[1], "OR") == 0)
      process_type = PT_OR;
    else {
      fputs ("Error: Unsupported gate type.\n", stderr);
      return 1;
    }
  }

  while ((c = getchar ()) != EOF) {
    bool read_operands = false;
    bool write_operands = false;
    if (isspace (c)) {
      if (c == '\n')
	line_num++;
      continue;
    } else if (c == 'N') {
      if (getchar () == 'A' && getchar () == 'N' && getchar () == 'D') {
	/* Found a `NAND' instruction, read/skip the three operands.  */
	read_operands = true;
	write_operands = (process_type == PT_NAND) ? true : false;
      } else SYNTAX_ERROR("Invalid instruction");
    } else if (c == 'O') {
      if (getchar () == 'R') {
	/* Found an `OR' instruction, read/skip the three operands.  */
	read_operands = true;
	write_operands = (process_type == PT_OR) ? true : false;
      } else SYNTAX_ERROR("Invalid instruction");
    } else SYNTAX_ERROR("Invalid instruction");
    if (read_operands) {
      unsigned long operands[3];
      unsigned i;
      for (i = 0; i < 3; i++) {
	while ((c = getchar ()) != EOF && isspace (c));
	if (c == EOF) SYNTAX_ERROR("Missing operand");
	ungetc (c, stdin);
	scanf ("%lu", &operands[i]);
      }
      /* Require a space after the last operand.  */
      if ((c = getchar ()) == EOF || !isspace (c))
	SYNTAX_ERROR("Missing space");
      if (write_operands) {
	/* Write out the instruction operands.  */
	for (i = 0; i < 3; i++) {
	  /* Determine the number of bits that need to be written.  */
	  unsigned bitwidth = 0;
	  unsigned j;
	  for (j = 0; j < 32; j++)
	    if (operands[i] & (1 << j)) bitwidth = j + 1;
	  { /* Check that our bit width is within our specified
	       limits.  */
	    unsigned byte_width = bitwidth >> 3 +
	      (bitwidth & (8 - 1)) ? 1 : 0;
	    if (byte_width > sizeof(unsigned))
	      SYNTAX_ERROR("Operand overflow");
	  }
	  /* Write out the operand.  */
	  WRITE_OPERAND(operands[i]);
	}
      }
    }
  }
  /* Flush any remaining buffered bits.  */
  if (bytebuf != 0) putchar (bytebuf);
  return 0;
}
