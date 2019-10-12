/* softvm.c -- A simple software-based implementation of virtual
   memory.

2019  */

/*

What are the key components of a software virtual memory design?

* Create instance -- Create a new virtual address space.
* Memory lock -- Pin pages to RAM at a fixed address for use.
* Memory unlock -- Unpin pages so they can be relocated and swapped.

* Translate address range -- Similar to Unix read() and write() system
  calls, but for virtual memory, so this doesn't copy data.
  Block-based, and may not return all memory requested.  The entire
  block returned is guaranteed to be contiguous.

  Kind of like svm_mlock(), but it does not require a matching unlock.
  Unlock is automatic and implicit after a certain number of
  svm_xlate() calls have transpired, determined by the size of the
  soft TLB.  You must not use a translated address after it has
  expired or else memory corruption will occur.

  If you would like a larger block of memory that is guaranteed to be
  in-order, use memory lock and memory unlock with the proper options.
  This comes with an up-front cost, of course.

* Copy buffers.  Exactly like read and write system calls.

* Truncate or expand memory.  Ability to expand VM space as unmapped
  pages.  Ability to map pages to free memory, or to map pages to
  existing physical addresses for the purpose of shared memory.

* Rearrange pages in existing mapping, insert pages between mapping,
  requires adequate unmapped page space.

* Destroy instance

* Jump execute memory.  Checks permissions on pages to see if execute
  is permitted for the first instruction, then jumps to that
  instruction.  Subsequent checks are performed only upon request.

* Call execute memory.  Checks permissions on pages to see if execute
  is permitted for the first instruction, then subroutine calls that
  instruction.  Subsequent checks are performed only upon request.

Yeah, so it's like I build on the implementation of files,
but make it more sophisticated.

Ultimately, since this is a full soft implementation, "compacting"
memory is a thing.  If users want contiguous blocks, then we incur
more overhead for that, of course.

But this obviously makes sense when you think about the limits of
early small computer systems, though.  With 4K pages, if you have 128K
of RAM, that means you only have 32 pages.  Seriously, that's nothing
for software memory management.  2 MB = 512 pages, 16 MB = 4096 pages.
At 24-bit address space, hardware virtual memory and paging is
helpful, but only when you enter true 32-bit address space does it
really shine.

For 16-bit addresses, 64K memory, well, virtual memory is plain
unnecessary.  So why am I implementing it?  If you are multitasking in
a 16-bit address space, which you will be under modern assumptions,
then virtual memory makes the implementation of multiple processes
easier.

*/
