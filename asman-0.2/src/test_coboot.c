#include <stdio.h>
#include <unistd.h>

#include "coboot.h"

void proc1(void);
void proc2(void);
void proc3(void);

/* PLEASE NOTE: Originally I was hoping to simply use nested C "scope"
   blocks to configure the stack allocations, but when testing with
   GCC, I found out that GCC coalesces these into one allocation at
   the top of the function.  Bummer.  Thus, I am using multiple
   functions here.  Also, the sad bloated standard C libraries that
   are the default installs on modern systems requires us to reserve
   quite a bit of stack space for each Coboot.  Though I must admit,
   it mamy also be the fault of the sad bloated Linux kernel.  */

void stub1(void)
{
	unsigned char padding1[8192*1];
	coboot_create(proc1);
}

void stub2(void)
{
	unsigned char padding2[8192*2];
	coboot_create(proc2);
}

void stub3(void)
{
	unsigned char padding3[8192*3];
	coboot_create(proc3);
}

void stub4(void)
{
	unsigned char guard[8192*4];
	puts("Initialization complete, ready to execute.");
	coboot_exec();
	puts("Exited gracefully.");
}

int main(void)
{
	unsigned char padding0[8192];
	coboot_init();
	stub1();
	stub2();
	stub3();
	stub4();
	return 0;
}

void proc1(void)
{
	unsigned char i;
	for (i = 0; i < 7; i++)
	{
		puts("I am process 1.");
		sleep(1);
		coboot_switch();
	}
	coboot_exit();
}

void proc2(void)
{
	unsigned char i;
	for (i = 0; i < 3; i++)
	{
		puts("They are process 2.");
		sleep(1);
		coboot_switch();
	}
	coboot_exit();
}

void proc3(void)
{
	unsigned char i;
	for (i = 0; i < 5; i++)
	{
		puts("It is process 3.");
		sleep(1);
		coboot_switch();
	}
	coboot_exit();
}
