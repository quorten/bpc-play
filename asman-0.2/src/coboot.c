/* coboot.c -- Simple implementation of cothreading.

Copyright (C) 2019 Andrew Makousky

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

/* This is primarily a simple implementation of cothreaded processes
   in C.  That is, context switching is voluntarily, in practice
   typically by waiting on I/O.  In principle, this is very similar to
   GNU Pth, GNU Portable Threads, but naturally, this implementation
   aims to be smaller and simpler.  */

/* Coboot -- An absolutely minimal implementation of cothreading,
   aiming to be suitable for inclusion in bootloader code.  */

#include <string.h>
#include <setjmp.h>

#include "coboot.h"

static jmp_buf g_exec_top;
static Coboot g_coboots_d[MAX_COBOOTS];
static unsigned char g_coboots_len;
/* N.B. `g_cur_coboot_id' is a very dicey variable that under certain
   usage, would need to be marked as `volatile'.  But, due to the
   simplicity of our Coboot implementation, it turns out we don't need
   to mark it as `volatile', so we don't do so for the sake of code
   readability and performance.  */
static unsigned char g_cur_coboot_id;

void coboot_init(void)
{
	g_coboots_len = 0;
}

/* Create a Coboot given a pointer to the start address.
   Conspicuously missing is a method to designate the allocated stack
   space.  This is simply copied from the current value, so you must
   allocate the proper amount of space by putting a variable of that
   size on the stack before calling this function.  This must include
   the stack space needed for the stub routines.

   The start routine itself must not return.  Instead, it should call
   `coboot_exit()'.  Otherwise, the code path that created the Coboot
   will be re-executed, which will likely result in a crash.  */
int coboot_create(void (*start)(void))
{
	void (*save_start)(void) = start;
	unsigned char coboot_id;
	if (g_coboots_len >= MAX_COBOOTS)
		return 0;
	coboot_id = g_coboots_len++;
	if (setjmp(g_coboots_d[coboot_id].regmem) == 0)
		return 1; /* Initialization complete.  */
	/* Otherwise, execute the start routine.  */
	/* return */ save_start();
}

void coboot_exec(void)
{
	g_cur_coboot_id = 0;
	/* N.B. For setjmp/longjmp, it is critical that we differentiate
	   whether we are entering or leaving.  When we destroy all
	   Coboots and return here, g_coboots_len will be zero.
	   Otherwise, it must be nonzero to switch to the first
	   Coboot.  */
	setjmp(g_exec_top);
	if (g_coboots_len > 0)
		longjmp(g_coboots_d[0].regmem, 1);
}

void coboot_switch_to(unsigned char coboot_id)
{
	unsigned char self_coboot_id = g_cur_coboot_id;
	g_cur_coboot_id = coboot_id;
	/* N.B. For setjmp/longjmp magic, it is critical that we
	   differentiate whether we are entering or leaving.  */
	if (setjmp(g_coboots_d[self_coboot_id].regmem) == 0)
		longjmp(g_coboots_d[g_cur_coboot_id].regmem, 1);
}

void coboot_switch(void)
{
	unsigned char coboot_id = g_cur_coboot_id + 1;
	/* N.B. We don't do modulo-division because if the hardware
	   instruction is not available, this is definitely faster.  */
	if (coboot_id >= g_coboots_len)
		coboot_id = 0;
	coboot_switch_to(coboot_id);
}

/* Switch to the next Coboot by means of a Return from Interrupt
   (RTI).  A pointer to the registers of the current Coboot that have
   been pushed onto the stack before the interrupt handler is
   required.  When called from a timer tick routine, this effectively
   allows for pre-emptive multitasking.  */
void coboot_switch_rti(jmp_buf *old_regmem)
{
	unsigned char self_coboot_id = g_cur_coboot_id;
	g_cur_coboot_id++;
	/* N.B. We don't do modulo-division because if the hardware
	   instruction is not available, this is definitely faster.  */
	if (g_cur_coboot_id >= g_coboots_len)
		g_cur_coboot_id = 0;

	memcpy(&g_coboots_d[self_coboot_id].regmem,
		   old_regmem, sizeof(jmp_buf));

	{ /* Load the registers of the next Coboot onto the top of the
		 stack, then do an interrupt return.  */
		unsigned char *top_stack = (unsigned char*) old_regmem;
		memcpy(top_stack, &g_coboots_d[g_cur_coboot_id].regmem,
			   sizeof(jmp_buf));
		top_stack += JMP_REGS_SZ;
		/* TODO: Consider implementing setjmp_irq and longjmp_rti
		   functions instead with the platform specifics.  */
		/* Not implemented!  */
		/* asm("movl {0}, %eax" : : : top_stack); */
		/* asm("iret"); */
	}
}

/* Destroy a coboot after it has finised running.  Please note that
   with this current implementation, this invalidates any saved
   references to Coboot IDs.  */
void coboot_destroy(unsigned char coboot_id)
{
	if (coboot_id >= g_coboots_len)
		return;
	/* N.B. We are basically copying code from exparray here, simply
	   so this code can be self-contained.  */
	memmove(g_coboots_d + coboot_id,
			g_coboots_d + coboot_id + 1,
			sizeof(Coboot) * (g_coboots_len - coboot_id + 1));
	g_coboots_len--;
	if (g_coboots_len == 0)
		longjmp(g_exec_top, 1);

	if (g_cur_coboot_id == coboot_id)
	{
		/* Since our own Coboot has ceased to exist, we must do
		   special handling for the switch.  */
		/* N.B. We don't do modulo-division because if the hardware
		   instruction is not available, this is definitely
		   faster.  */
		if (g_cur_coboot_id >= g_coboots_len)
			g_cur_coboot_id = 0;
		longjmp(g_coboots_d[g_cur_coboot_id].regmem, 1);
	}
	else
	{
		if (g_cur_coboot_id > coboot_id)
			g_cur_coboot_id--;
		coboot_switch();
	}
}

/* DO NOT save the return value long-term inside of user code.  Old
   Coboot IDs are not guaranteed to be valid after a call to
   `coboot_destroy()'.  */
unsigned char coboot_gettid(void)
{
	return g_cur_coboot_id;
}

/* Exit the current Coboot.  */
void
coboot_exit(void)
{
	return coboot_destroy(coboot_gettid());
}
