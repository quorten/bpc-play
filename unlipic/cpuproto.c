/* CPU rapid prototyping framework.  The quick and dirty way to define
   an instruction set architecture and the behavior of individual
   instructions, without regard to the final instruction encoding.  A
   pseudo instruction encoding is invented on the fly.  */

/*

So, how does this work?  There are two main sections, the
parser-assembler and the byte code interpreter.

The byte code interpreter is rather simple: Read a single 16-bit
opcode, read the corresponding parameters data structure (if any), and
then call the corresponding subroutine, giving it a pointer to the
parameters data structure.  That's all there is to it!

The parser-assembler is a bit more complicated, but not too much.
Parse and store address labels, of course.  Specify the base address,
i.e. ORG pseudo-instruction.  Equate and macro processing is provided
separately.  The instruction name is read up to the first whitespace
character, and then a table is used to find out which subroutine to
call to parse the remainder of the line.  The next steps are all up to
the subroutine that parsed the remainder of the line, but typically
this will be, of course, to select an opcode and pack up the
parameters data structure.

Okay, here's actually a really good idea to pursue.  The opcode itself
is defined as part of the parameters data structure.  This means the
bytecode interpreter basically starts out by reading the first part of
the parameters data structure, then figuring out how much more needs
to be read to get the remainder.  Now this allows for customizability
in how long the opcode is, and you can even use bitfields as you
please.

Now, as you can see, this is more than just rapid prototyping.  No,
this is for full stack development.

Hardware and device control.  Unlike gate-sim, we're trying to use a
simpler model where there is not as much simulator overhead, at the
expense of accurate simulation.  So, this is how it works.

There are two separate kinds of memory operations, pure memory
operations and device address space operations.  Pure memory
operations are simply straight memory array read and writes.  Device
address space reads and writes go through.  Program instructions are
always pure memory reads.  For individual instruction execution, it is
up to the processor designer which method to use.  A processor
designer might opt to use special device address space read/write
instructions ("I/O-mapped I/O") to speed up simulation rather than
making every memory access a potential device address space access.

Finally, the ne ultra fastest simulation model is to emit C code that
is compiled and executed directly rather than interpreted byte codes.
This is easy with the simulator framework since the execution
subroutines are already defined, it's just a matter of packing the
instruction parameters and calling the subroutines in sequence.  Link
with the simulator library, and you're all set.

Interrupts and GPIO?  Well, again that's totally up to you how you
want to implement those.  This framework dictates no such
requirements.  If you want to support hardware devices in "JIT mode"
either you set a compile flag to check after every instruction, issue
a special instruction in your code to simulate hardware and interrupts
conditions.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exparray.h"

/* N.B.: Change the width of Opcode here depending on your
   requirements.  */
typedef unsigned short Opcode;
/* Maximum instruction length in bytes.  */
#define MAX_INSTR_LEN 32
/* Maximum mnemonic length in bytes, including the null character.  */
#define MAX_MNEM_LEN 32
/* AND mask to apply when decoding an opcode, use this if there are
   bit-fields in the opcode that the executor will decode itself.  */
#define OPCODE_MASK 0xffff

/* N.B. Adjust this based off of the desired address width of your
   simulated CPU, this is used to set the variable for the program
   counter (PC) register to the correct width.  */
typedef unsigned int SimAddr;

typedef void (*OpExecutor)(void*);
typedef int (*LineParser)(FILE*);

/* Opcode information, used for processor simulation.  */
struct OpcodeInfo_tag
{
  Opcode op;
  unsigned char instr_len;
  /* Subroutine to execute this instruction.  */
  OpExecutor executor;
};
typedef struct OpcodeInfo_tag OpcodeInfo;

/* Mnemonic information, used for assembly.  */
struct MnemonicInfo_tag
{
  char *prefix;
  /* Subroutine to parse the remainder of this line.  */
  LineParser parser;
};
typedef struct MnemonicInfo_tag MnemonicInfo;

typedef unsigned char byte;

EA_TYPE(OpcodeInfo);
EA_TYPE(MnemonicInfo);
EA_TYPE(byte);

OpcodeInfo_array g_opcode_tbl;
MnemonicInfo_array g_parse_tbl;

/* Main memory for processor simulation.  */
unsigned char *g_memory = NULL;
/* Program counter register, i.e. current/next instruction.  */
SimAddr g_reg_pc = 0;
/* All other registers are defined in global variables of your own
   choosing.  */

/* Current line number in assembly input.  */
unsigned g_line_num = 0;
/* Base address of loaded machine code.  */
SimAddr g_asm_base;
/* Assember-generated machine code.  */
byte_array g_asm_out;

/* Decode and execute the current instruction.  Basically, one step of
   the processor simulation.  */
void
cpu_sim_step (void)
{
  /* Fetch, decode the opcode, fetch the arguments.  Note that the
     opcode is included in the instruction parameters data structure,
     so basically it is a C struct of the full instruction bytes.  */
  void *instr_params = &g_memory[g_reg_pc];
  Opcode decode_op = *(Opcode*)instr_params & OPCODE_MASK;
  OpcodeInfo *info = &g_opcode_tbl.d[decode_op];
  g_reg_pc += info->instr_len;
  /* Execute the instruction.  */
  info->executor (instr_params);
}

int
instr_compare_fn (void *a, void *b)
{
  return strcmp ((char*)a, (char*)b);
}

/* Parse the beginning of an assembly language line and determine
   which parser subroutine to call next.  */
int
parse_line_start (FILE *fp)
{
  int c;
  char prefix_d[MAX_MNEM_LEN];
  unsigned prefix_len;
  unsigned i;

  /* Skip leading whitespace.  */
  while ((c = getc (fp)) != EOF) {
    if (isspace (c)) {
      if (c == '\n') {
	g_line_num++;
	return 1;
      }
    } else
      break;
  }
  if (c == EOF)
    return EOF;
  prefix_d[prefix_len++] = (char)c;

  /* Read the rest of the prefix up to the whitespace, and no more
     than the max mnemonic length.  We treat parentheses as whitespace
     so that we can support "functional notation" assembly language
     instructions.  */
  while ((c = getc (fp)) != EOF && prefix_len < MAX_MNEM_LEN - 1) {
    if (isspace (c)) {
      if (c == '\n') {
	g_line_num++;
      }
      break;
    } else if (c == '(' || c == ')')
      break;
    else
      prefix_d[prefix_len++] = (char)c;
  }
  prefix_d[prefix_len] = '\0';

  /* Search for the subroutine to parse the remainder of the line.  */
  /* TODO FIXME: Change this to binary search, this is going to have
     to be `bsearch_ordered ()` due to the different data types.  */
  for (i = 0; i < g_parse_tbl.len; i++) {
    if (strcmp (prefix_d, g_parse_tbl.d[i].prefix) == 0) {
      return g_parse_tbl.d[i].parser(fp);
    }
  }

  /* No prefix match found?  That's an error.  */
  fprintf (stderr, "Syntax error on line %u: %s\n", g_line_num,
	   "Invalid instruction");
  return EOF;
}

int
main (int argc, char *argv[])
{
  g_memory = malloc (sizeof(char) * 65536);
  if (g_memory == NULL)
    return 1;
  EA_INIT(OpcodeInfo, g_opcode_tbl, 16);
  EA_INIT(MnemonicInfo, g_parse_tbl, 16);
  EA_INIT(byte, g_asm_out, 65536);

  /* Assemble the input source.  */
  while (parse_line_start (stdin) != EOF);

  /* Load it to the desired base address in memory.  */
  memcpy (&g_memory[g_asm_base], g_asm_out.d, g_asm_out.len);

  /* Now just execute it right away for demo purposes.  */
  while (1)
    cpu_sim_step ();

  free (g_memory);
  EA_DESTROY(g_opcode_tbl);
  EA_DESTROY(g_parse_tbl);
  EA_DESTROY(g_asm_out);

  return 0;
}
