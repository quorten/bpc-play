#ifndef COBOOT_H
#define COBOOT_H

#include <setjmp.h>

/* TODO FIX THESE!  */
#define JMP_REGS_SZ 12

#define MAX_COBOOTS 16
#define MAX_COPROCS 512

struct Coboot_tag
{
	jmp_buf regmem;
};
typedef struct Coboot_tag Coboot;

void coboot_init(void);
int coboot_create(void (*start)(void));
void coboot_exec(void);
void coboot_switch_to(unsigned char coboot_id);
void coboot_switch(void);
void coboot_switch_rti(jmp_buf *old_regmem);
void coboot_destroy(unsigned char coboot_id);
unsigned char coboot_gettid(void);
void coboot_exit(void);

#endif /* not COBOOT_H */
