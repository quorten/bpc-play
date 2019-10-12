/* Simulate a circuit composed entirely out of NAND and OR gates.  See
   `~/inven-sys.txt', `notes-gate-sim.txt', and my paper notes for
   related documentation.  I'm too lazy and in too much of a hurry to
   do any more formalism here.

   Authored in 2015, 2018, 2019 by Andrew Makousky

   This file is in Public Domain.  I'm too lazy to write any more
   legalese.

*/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
/* For `srand (time (NULL))': */
#include <time.h>

/* For UART hardware */
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>

#include "bool.h"
#include "exparray.h"

/* Implement an array of wire values.
   Implement an array of gates.

   Read the gates.
   On every cycle, compute the value at every single gate and
   update the wire values.
   On every cycle, simulate the virtual hardware devices.

*/

#define BIT_ARRAY_GET(array, pos) \
  ((array)[(pos)>>3] >> ((pos) & (8 - 1)))
#define BIT_ARRAY_GET_BYTE(array, pos) \
  ((array)[(pos)>>3])

/* If clock is zero, do nothing.  Otherwise, it should be set to 1, or
   else the strange things will happen.  */
#define BIT_ARRAY_SET(array, pos, clock) \
  ((array)[(pos)>>3] |=  ((clock) << ((pos) & (8 - 1))))

#define BIT_ARRAY_CLEAR(array, pos, clock) \
  ((array)[(pos)>>3] &= ~((clock) << ((pos) & (8 - 1))))

struct Gate_tag
{
  /* All of these indexes are wire numbers.  */
  unsigned in1;
  unsigned in2;
  unsigned out;
};
typedef struct Gate_tag Gate;
EA_TYPE(Gate);

/* We switch back and forth between two wire buffers so that we don't
   mix up old values with new values.  That is, we only read the from
   old values and write to new values within a time frame.  */
unsigned char *wires[2];
unsigned num_wires;
Gate_array nand_gates;
Gate_array or_gates;
unsigned sim_time = 0;
unsigned reset_cycles;
unsigned clock_cycles;
unsigned shutdown_cycles;
/* #define RESET_PERIOD 32 */
#define RESET_PERIOD 128
/* #define CLOCK_PERIOD 64 */
#define CLOCK_PERIOD 32
struct termios old_termios;

/* TODO: Add support for hardware-global state.  So that we don't have
   to ever compare old wires to new wires.  Read from old wires, write
   to new wires.  */
/* UART global state */
unsigned char uart_ik_prev;
unsigned char uart_ok_prev;
unsigned uart_ok_cycles;
#define UART_CLOCK_PERIOD 8

/* Returns 2 on EOF, otherwise return the bit value.  */
unsigned char
file_read_bit (FILE *fp)
{
  static unsigned char bit_pos = 0;
  static int ch;
  unsigned char bit;

  if (bit_pos == 0) {
    ch = getc (fp);
    if (ch == EOF)
      return 2;
  }
  bit = (ch >> bit_pos) & 1;
  bit_pos = ++bit_pos & (8 - 1);
  return bit;
}

/* Obsolete.  Read a variable-width unsigned integer that uses leading
   "1" marks to indicate a bit, "0" marks to indicate a space.  On
   EOF, returns UINT_MAX.  On error, such as invalid read or overflow,
   returns UINT_MAX - 1.  */
unsigned
old_file_read_uint (FILE *fp)
{
  /* First skip any leading spaces.
     Loop:
     1. Read the mark/space indicator.
     2. Read the bit value.
  */
  unsigned value = 0;
  unsigned place = 0;
  unsigned char is_mark;
  while ((is_mark = file_read_bit (fp)) == 0);
  while (is_mark == 1) {
    unsigned char bit = file_read_bit (fp);
    if (bit == 2)
      return UINT_MAX - 1;
    if (place >= 32)
      return UINT_MAX - 1;
    value |= bit << place++;
    is_mark = file_read_bit (fp);
  }
  if (is_mark == 2)
    return UINT_MAX;
  return value;
}

/* Read a fixed-width little endian unsigned integer.  On EOF, returns
   UINT_MAX.  On error, such as invalid read or overflow, returns
   UINT_MAX - 1.  */
unsigned
file_read_uint (FILE *fp)
{
  unsigned result = 0;
  unsigned char i;
  for (i = 0; i < sizeof (unsigned); i++) {
    int ch = getc (fp);
    if (ch == EOF)
      return UINT_MAX;
    result |= ch << (i * 8);
    /* result |= ch << (i + (i << 1)); */
  }
  return result;
}

bool
file_read_gates (Gate_array *gates, FILE *fp)
{
  /* Read all the gates into the array, keeping track of the max wire
     value.  */
  unsigned value = file_read_uint (fp);
  unsigned max_wire = 0;
  EA_INIT(Gate, *gates, 16);
  while (value != UINT_MAX) {
    Gate *gate = &gates->d[gates->len];

    if (value == UINT_MAX - 1)
      return false;
    /* if (value > max_wire + 1)
      return false; */ /* Wires must be consecutive.  */
    if (value > max_wire)
      max_wire = value;
    gate->in1 = value;

    value = file_read_uint (fp);
    if (value == UINT_MAX || value == UINT_MAX - 1)
      return false;
    if (value > max_wire)
      max_wire = value;
    gate->in2 = value;

    value = file_read_uint (fp);
    if (value == UINT_MAX || value == UINT_MAX - 1)
      return false;
    if (value > max_wire)
      max_wire = value;
    gate->out = value;

    EA_ADD(*gates);
    value = file_read_uint (fp);
  }

  if (max_wire + 1 > num_wires)
    num_wires = max_wire + 1;
  return true;
}

bool
alloc_wires ()
{
  /* Allocate the wires array.  */
  unsigned long wire_size = (num_wires >> 3) + 1;
  if (wires[0] != NULL) free (wires[0]);
  if (wires[1] != NULL) free (wires[1]);
  wires[0] = (char *)malloc (wire_size);
  wires[1] = (char *)malloc (wire_size);
  if (wires[0] == NULL || wires[1] == NULL)
    return false;
  /* Please note that for realistic simulation, we want random data
     values on the wires at the start of the simulation.  For most
     architectures, this is the default.  Nevertheless, for all
     architectures, explicitly generated pseudo-random data makes the
     space much more random.  */
  {
    unsigned long i;
    time_t seed = time (NULL);
    /* Use seed 1562283516 (ROSC16) or 1562299292 (ROSC32) or
       1562387592 (ROSC32) or 1562399647 (ROSC32) or 1562423137
       (ROSC32) with advcount to reproduce "fibrillation" issue.  */
    fprintf (stderr, "Seed is %lu\n", seed);
    srand (seed);
    for (i = 0; i < wire_size; i++) {
      /* N.B. We initialize the second pair of wires to be opposite
	 the of the first as a cheap way to simulate random noise on
	 floating CMOS inputs.  */
      /* TODO: Add feature for deterministic initialization, that can
	 also help find some otherwise hard-to-find bugs.  */
      unsigned char rand_val = (unsigned char)rand ();
      wires[0][i] = rand_val;
      wires[1][i] = ~rand_val;
      /* wires[0][i] = rand () & 0xff;
      wires[1][i] = rand () & 0xff; */
    }
  }
  /* TODO: New feature: Check simulation mode, set or accumulate.  Are
     implicit OR gates needed?  */
  /* TODO: New feature: Set floating input wires to random data.
     Simulates floating CMOS inputs.  */
  /* TODO: New feature: Built-in always one (Vcc) and always zero
     (GND).  Because, if we're CMOS, we can't derive, unless we cheat
     our simulation with a one-gate delay circuit to build consistency
     out of chaos.  */
  return true;
}

void
sim_step ()
{
  /* Compute the value at every single gate and update the wire
     values.  */
  unsigned char *old_wires = wires[(sim_time & 1)];
  unsigned char *new_wires = wires[((sim_time + 1) & 1)];
  unsigned i;
  /* These looks like very tight inner loops where assembly language
     would be ideal.  It also looks ideal for GPGPU parallel
     computing.  Hence, we have a little bit of "loop unrolling" going
     on here, although it makes the code a little repetitive.  */
  for (i = 0; i < nand_gates.len; i++) {
    unsigned char accum = BIT_ARRAY_GET(old_wires, nand_gates.d[i].in1);
    accum &= BIT_ARRAY_GET(old_wires, nand_gates.d[i].in2);
    accum = ~accum & 1;
    BIT_ARRAY_CLEAR(new_wires, nand_gates.d[i].out, 1);
    BIT_ARRAY_SET(new_wires, nand_gates.d[i].out, accum);
  }
  for (i = 0; i < or_gates.len; i++) {
    unsigned char accum = BIT_ARRAY_GET(old_wires, or_gates.d[i].in1);
    accum |= BIT_ARRAY_GET(old_wires, or_gates.d[i].in2);
    accum &= 1;
    BIT_ARRAY_CLEAR(new_wires, or_gates.d[i].out, 1);
    BIT_ARRAY_SET(new_wires, or_gates.d[i].out, accum);
  }
}

bool
signals_init ()
{
  unsigned reset_wire = 21;
  unsigned clock_wire = 22;
  unsigned shutdown_wire = 23;
  unsigned char old_wires = (unsigned char)(sim_time & 1);
  unsigned char new_wires = (unsigned char)((sim_time + 1) & 1);
  reset_cycles = RESET_PERIOD;
  clock_cycles = 0;
  shutdown_cycles = 0;
  BIT_ARRAY_SET(wires[old_wires], reset_wire, 1);
  BIT_ARRAY_SET(wires[new_wires], reset_wire, 1);
  BIT_ARRAY_CLEAR(wires[old_wires], clock_wire, 1);
  BIT_ARRAY_CLEAR(wires[new_wires], clock_wire, 1);
  BIT_ARRAY_CLEAR(wires[old_wires], shutdown_wire, 1);
  BIT_ARRAY_CLEAR(wires[new_wires], shutdown_wire, 1);
  return true;
}

/* Simulate essential signals for digital sequential logic.

   * RESET signal
   * (optional) CLOCK signal
   * (optional) SHUTDOWN signal, turn off the power and end the
     simulation.

   Wiring:

   RESET wire = 21
   CLOCK wire = 22
   SHUTDOWN wire = 23

*/
bool
signals_step ()
{
  unsigned reset_wire = 21;
  unsigned clock_wire = 22;
  unsigned shutdown_wire = 23;
  unsigned char old_wires = (unsigned char)(sim_time & 1);
  unsigned char new_wires = (unsigned char)((sim_time + 1) & 1);
  if (reset_cycles > 0) {
    reset_cycles--;
  }
  if (reset_cycles == 0) {
    unsigned char shutdown_val = BIT_ARRAY_GET(wires[old_wires], shutdown_wire);
    BIT_ARRAY_CLEAR(wires[new_wires], reset_wire, 1);
    /* Ignore the value of the SHUTDOWN signal until the RESET signal
       goes low.  */
    if (shutdown_val) {
      shutdown_cycles++;
    } else
      shutdown_cycles = 0;
  } else {
    BIT_ARRAY_SET(wires[new_wires], reset_wire, 1);
    BIT_ARRAY_CLEAR(wires[new_wires], shutdown_wire, 1);
  }
  clock_cycles = (clock_cycles + 1) & (CLOCK_PERIOD - 1);
  if (clock_cycles >= CLOCK_PERIOD / 2)
    BIT_ARRAY_SET(wires[new_wires], clock_wire, 1);
  else
    BIT_ARRAY_CLEAR(wires[new_wires], clock_wire, 1);
  return true;
}

bool
uart_init ()
{
  unsigned base_wire = 0;
  unsigned ik_wire = base_wire + 16 + 0;
  unsigned ok_wire = base_wire + 16 + 1;
  unsigned iq_wire = base_wire + 16 + 2;
  unsigned oq_wire = base_wire + 16 + 3;
  unsigned ir_wire = base_wire + 16 + 4;

  unsigned char old_wires = (unsigned char)(sim_time & 1);
  unsigned char new_wires = (unsigned char)((sim_time + 1) & 1);

  /* We must zero the toggle wires or else things will go awry.  */
  BIT_ARRAY_CLEAR(wires[old_wires], ik_wire, 1);
  BIT_ARRAY_CLEAR(wires[old_wires], ok_wire, 1);

  /* Set sane defaults for input ready, output ready, and input
     overrun.  */
  BIT_ARRAY_CLEAR(wires[old_wires], iq_wire, 1);
  BIT_ARRAY_SET(wires[old_wires], oq_wire, 1);
  BIT_ARRAY_CLEAR(wires[old_wires], ir_wire, 1);

  uart_ik_prev = 0;
  uart_ok_prev = 0;
  uart_ok_cycles = 0;

  return true;
}

bool
uart_resume ()
{
  /* Configure unbuffered, non-echo input on our terminal.  For basic
     use, we don't want to configure a totally raw terminal.  */
  if (isatty (STDIN_FILENO)) {
    struct termios settings;
    int result = tcgetattr (STDIN_FILENO, &old_termios);
    if (result < 0) {
      perror ("error getting terminal flags");
      return false;
    }
    memcpy (&settings, &old_termios, sizeof (struct termios));
    settings.c_lflag &= ~(ICANON | ECHO);
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 0;
    result = tcsetattr (STDIN_FILENO, TCSANOW, &settings);
    if (result < 0) {
      perror ("error setting terminal flags");
      return false;
    }
  } else
    fputs ("Not a typewriter.\n", stderr);

  { /* Configure non-blocking mode on standard input.  */
    int fflags = fcntl (STDIN_FILENO, F_GETFL);
    int result;
    if (fflags == -1) {
      perror ("error getting stdin flags");
      return false;
    }
    fflags |= O_NONBLOCK;
    result = fcntl (STDIN_FILENO, F_SETFL, fflags);
    if (result == -1) {
      perror ("error setting stdin flags");
      return false;
    }
  }

  { /* Eliminate buffering on higher-level I/O.  */
    int result = setvbuf (stdin, NULL, _IONBF, 0);
    if (result != 0) {
      perror ("error clearing input buffering");
      return false;
    }
    result = setvbuf (stdout, NULL, _IONBF, 0);
    if (result != 0) {
      perror ("error clearing output buffering");
      return false;
    }
  }

  return true;
}

bool
uart_suspend ()
{
  /* Reset our terminal back to normal.  */
  if (isatty (STDIN_FILENO)) {
    int result = tcsetattr (STDIN_FILENO, TCSANOW, &old_termios);
    if (result < 0) {
      perror ("error setting terminal flags");
      return false;
    }
  }
  return true;
}

bool
uart_shutdown ()
{
  return true;
}

/* Simulate a 8-bit UART hardware device that is wired up to standard
   input and standard output.  Returns true on success, false on
   hardware failure (i.e. EOF).

   The least significant bit comes first on hardware output
   interpretation.

   The baud rate determines the time period for one symbol.  To avoid
   data loss, the system must respond to input within this time period
   and must not write output faster than this time period.

   TODO NOTE: Another option to consider.  Rather than execute on both
   the rising and falling edges of the clocks, we could execute on
   only the rising edge.

   Wiring:

   UART base wire = 0

   |                       |                      |-- Control ---|
   |-------- Input --------|------- Output -------|IK OK IQ OQ IR|
    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20

   Control channel:

   Input bit: IK: "Input clock."  Toggle this bit to indicate the
   system is ready to receive.

   Input bit: OK: "Output clock."  Toggle this bit to indicate the
   system is ready to send.

   Output bit: IQ: "Input interrupt."  This bit is set when there is
   input data available.  Once acknowledged with IK, this bit will be
   cleared.

   Output bit: OQ: "Output interrupt."  This bit is set when the UART
   is ready send.  Once acknowledged with OK, this bit will be cleared
   until the time period for one symbol has passed.

   Output bit: IR: "Input overrun."  This bit is set when the system
   has failed to read input fast enough and some input was lost.  It
   is cleared on the next time the input clock is toggled.

*/
bool
uart_step ()
{
  /* Important!  The base wire must be set so that the input and
     output wires are aligned on an 8-bit byte.  */
  unsigned base_wire = 0;
  unsigned input_base_wire = base_wire + 0;
  unsigned output_base_wire = base_wire + 8;
  unsigned ik_wire = base_wire + 16 + 0;
  unsigned ok_wire = base_wire + 16 + 1;
  unsigned iq_wire = base_wire + 16 + 2;
  unsigned oq_wire = base_wire + 16 + 3;
  unsigned ir_wire = base_wire + 16 + 4;

  unsigned char old_wires = (unsigned char)(sim_time & 1);
  unsigned char new_wires = (unsigned char)((sim_time + 1) & 1);

  unsigned char ik_val = BIT_ARRAY_GET(wires[old_wires], ik_wire) & 1;
  unsigned char ok_val = BIT_ARRAY_GET(wires[old_wires], ok_wire) & 1;
  unsigned char iq_val = BIT_ARRAY_GET(wires[old_wires], iq_wire) & 1;

  if (uart_ik_prev != ik_val) {
    /* Set no input available.  */
    BIT_ARRAY_CLEAR(wires[new_wires], iq_wire, 1);
    iq_val = 0;
    /* Clear the input overrun wire.  */
    BIT_ARRAY_CLEAR(wires[new_wires], ir_wire, 1);
  }
  /* TODO: If input is not coming from a terminal, only read input at
     the max baud rate.  */
  { /* Check if input is available.  */
    unsigned char old_input_val = BIT_ARRAY_GET_BYTE(wires[old_wires],
						     input_base_wire);
    unsigned char *input_val = &BIT_ARRAY_GET_BYTE(wires[new_wires],
						   input_base_wire);
    int ch = getc (stdin);
    /* Copy old to new regardless of whether input is available.  */
    *input_val = old_input_val;
    if (ch == EOF) {
      if (ferror (stdin)) {
	if (errno == EWOULDBLOCK) {
	  ; /* Continue to output processing.  */
	} else {
	  perror ("UART read error");
	  return false;
	}
      }
      if (feof (stdin)) {
	fputs ("UART end of file.\n", stderr);
	return false;
      }
    } else {
      if (ch == '\x04') {
	/* End of file on terminal. */
	fputs ("UART end of file.\n", stderr);
	return false;
      }
      *input_val = (char)ch;
      BIT_ARRAY_SET(wires[new_wires], iq_wire, 1);
      /* Check if the input overrun wire should be signaled.  */
      if (iq_val)
	BIT_ARRAY_SET(wires[new_wires], ir_wire, 1);
    }
  }
  /* If enough time has passed, set the OQ wire.  */
  if (uart_ok_cycles >= UART_CLOCK_PERIOD)
    BIT_ARRAY_SET(wires[new_wires], oq_wire, 1);
  else
    uart_ok_cycles++;
  if (uart_ok_prev != ok_val) {
    unsigned char output_val = BIT_ARRAY_GET_BYTE(wires[old_wires],
						  output_base_wire);
    if (putc (output_val, stdout) == EOF)
      return false;
    BIT_ARRAY_CLEAR(wires[new_wires], oq_wire, 1);
    uart_ok_cycles = 0;
  }

  /* Update our memory of the previous IK and OK values.  */
  uart_ik_prev = ik_val;
  uart_ok_prev = ok_val;

  return true;
}

/* Perform hardware device initialization that only needs to be done
   at the very beginning of a simulation.  This generally means
   setting initial hardware wire values.  */
bool
hw_init ()
{
  if (!signals_init ())
    return false;
  if (!uart_init ())
    return false;
  return true;
}

/* Initialize hardware devices when resuming a simulation.  */
bool
hw_resume ()
{
  if (!uart_resume ())
    return false;
  return true;
}

/* Process one simulation step for virtual hardware devices,
   i.e. anything that communicates with the external world using
   physics.  Returns true on success, false on hardware failure.  */
bool
hw_step ()
{
  /* TODO: External clock signal generator, in case you do not want to
     generate your own clock signal using a ring oscillator and a
     counter circuit for a frequency divider.  */
  /* TODO: Hardware reset signal, because sequential logic can never
     truly initialize itself to a deterministic state from a random
     state.  */
  if (!signals_step ())
    return false;
  if (!uart_step ())
    return false;
  /* TODO: Atari-style digital video output circuit.  */
  /* TODO: Video character/bitmap framebuffer.  */
  /* TODO: One-bit sound output.  */
  /* TODO: Keyboard input.  */
  /* TODO: Mouse/joystick input.  */
  /* TODO: Disk I/O interface.  */
  /* TODO: Pulse-width modulation sound output.  */
  /* TODO: MIDI synthesizer.  */
  /* TODO: PCM sound input/output.  */

  /* That's all we can cover for the basics.  The rest involves more
     advanced hardware that is generally highly custom and accessed
     through the use of a device driver on the operating system
     side.  */
  /* USB might be the only exception.  */
  return true;
}

/* Perform hardware shutdown required before exiting the
   simulator.  */
bool
hw_suspend ()
{
  bool result = true;
  if (!uart_suspend ())
    result = false;
  return result;
}

/* De-initialize hardware completely at the end of a simulation.  */
bool
hw_shutdown ()
{
  return true;
}

int
main (int argc, char *argv[])
{
  int retval = 0;
  FILE *fp;

  if (argc != 2 && argc != 3) {
    fputs ("Invalid command  line\n", stderr);
    return 1;
  }

  fp = fopen (argv[1], "rb");
  if (fp == NULL) {
    fputs ("Error opening NAND gates file\n", stderr);
    return 1;
  }
  if (!file_read_gates (&nand_gates, fp)) {
    fputs ("Error reading NAND gates\n", stderr);
    return 1;
  }
  fclose (fp);
  if (argc == 3) {
    fp = fopen (argv[2], "rb");
    if (fp == NULL) {
      fputs ("Error opening OR gates file\n", stderr);
      return 1;
    }
    if (!file_read_gates (&or_gates, fp)) {
      fputs ("Error reading OR gates\n", stderr);
      return 1;
    }
    fclose (fp);
  } else {
    or_gates.d = NULL;
    or_gates.len = 0;
  }
  if (!alloc_wires ()) {
    fputs ("Error allocating wires\n", stderr);
    return 1;
  }
  if (!hw_init ()) {
    fputs ("Error initializing hardware.\n", stderr);
    return 1;
  }
  if (!hw_resume ()) {
    fputs ("Error resuming hardware.\n", stderr);
    return 1;
  }

  while (1) {
    sim_step ();
    if (!hw_step ()) {
      fputs ("Hardware simulation error.\n", stderr);
      retval = 1;
      break;
    }
    /* Special case: Check for digital shutdown signal, and break loop
       cleanly if so.  The shutdown wire must be held steady for a
       reasonably long period, such as the period of the RESET
       signal.  */
    if (shutdown_cycles >= RESET_PERIOD)
      { putchar ('\n'); break; }
    sim_time++;
  }

 cleanup:
  hw_suspend ();
  hw_shutdown ();
  EA_DESTROY(nand_gates);
  EA_DESTROY(or_gates);
  return retval;
}
