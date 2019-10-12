/* This is the object definition for the "system memory" of the
   bootstrap monitor, i.e. the start of all code in the boot system.
   Please note that we set type to "function" so that we can see
   the address in disassemble listings.  */
.section .text
.global memory
.type memory, @function
/* .align 65536, 0x00 */
memory:
