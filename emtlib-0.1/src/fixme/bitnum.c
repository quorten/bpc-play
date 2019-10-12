/* For my experimental programs, I need to write a small library to
   work with arbitrary precision numbers.  It will have only three
   functions: shift left, shift right, and add.  It will also only
   support unsigned numbers.  Most significant bits come last.  (Note
   that you can change this behavior.)  That way it is easier to
   expand the number in memory.  The actual number data is preceded by
   an exponential continuation header, as I have specified in my
   Artificial Intelligence papers.  */

/* There is no such thing as a platform-independent implementation of
   this library.  The only platform-independent part is the external
   interface.  The native pointer implementation must vary between
   platforms.  */

/* This version is for 32-bit x86 CPUs, flat memory model only.
   Native pointers are 32 bits in size.  Segmented memory access is
   not supported.  Explicit hard drive swapping is not supported.  */

/* There are two implementations of Bitnums: "real" Bitnums and
   architecture-accelerated (or native) Bitnums.  */

/* How does a typical Bitnum programming workflow work?

   * First allocate or load a Bitnum.

   * Call the function `bn_to_aabn()' for higher performance during
     computations.

   * Perform your computations.

   * Call the function `aabn_to_bn()' before saving to disk or sending
     across a network.

   * There are also functions to load a bitnum from a stream.  */

/* Provide more functions to work with native Bitnums
   directly.  */

/* If a Bitnum's size header is too big, throw an exception like a
   hardware processor would.  */

/* If the algorithms compute a number bigger than the hardware
   handles, the software can configure different behavior to happen:

   * Wrap around (default).

   * Throw an exception.

   * Perform saturation arithmetic (silent truncation to the limits).
     You are strongly discouraged from using this technique, as it is
     only applicable to a limited number of special cases.  You should
     instead write special-case code to handle those cases.  */

/* Provide primitives for Bitnum arrays.  */

/* Provide a native number translator.  */

/* Structure of a native Bitnum:

   * All bits in a native Bitnum byte go in hardware ascending order.
     The least significant bit of a native Bitnum is the least
     significant bit of a native byte.

   * Bytes inside of a native Bitnum are grouped by the word size of
     the hardware architecture.  Byte ordering within these groups is
     the same as the native architecture.

   * The size header of a native Bitnum is padded to the word size of
     the hardware architecture with extra zeros.

   * The word groups then proceed forward in memory from least
     significant word group to the most significant word group.  */

/* To convert a native Bitnum to a Bitnum:

   * I use to say to reverse the bit ordering within each byte.  Now
     this is no longer the case.  Rather than telling people to do
     this for real architectures, I am now going to say that for my
     abstract architecture, if bytes are to be represented, the most
     significant bit of a byte will be highest in memory.  Besides,
     modern hardware has no specification for the bit ordering of
     bytes in hardware memory, just that the most significant bit and
     least significant bit are consistent during data interchange.
     (To my knowledge, most hardware is designed with the most
     significant bit being the leftmost bit.)

   * Swap the byte ordering within each word group, if necessary.  */

/* One last word: Although in theory, the idea of putting least
   significant bits and bytes first may seem to reduce allocation
   problems, in practice, this is not so.  Most programs work with
   multiple numbers, and each number must have a fixed amount of space
   pre-allocated in advance.  If a number must exceed this
   pre-allocated space, then is has to be completely copied to a new
   memory space.  An optimizing implementation that works with big
   endian numbers will copy the number to the end of the allocated
   space.  So in the standard case and optimized software, there is no
   performance gain from using little endian rather than big endian.
   Generally, little-endian will only be faster than big-endian with
   naive non-optimized implementations.  But alas, even if this is a
   small performance gain, it is still performance to be gained.
   However, in the case of Bitnums, there is a fixed size header
   before the actual number data that will also need to get expanded.
   This would still result in the following data getting moved, in a
   possibly redundant manner, so alas, the gains are not
   available.  */

/* Not so fast!  There's more than one way to specify an arbitrary
   precision number.  Rather than specifying an exponential header,
   exponential continuation bits could be interleaved throughout the
   number.  This way, redundant memory moves could be avoided.
   However, this would also complicate other computations because the
   number would have to be parsed more deeply in order to find out the
   number of bits the number uses.  This is the traditional memory
   versus speed tradeoff.  Another possibility is to use linear
   continuations rather than exponential continuations.  And of
   course, there is the traditional possibility of using a terminating
   zero and encoding the data to not use such a sequence of bytes
   before the terminator.  */

/* NOTE: I should really document data continuation, encoding methods,
   and efficiency calculations.  */

typedef unsigned char *Bitnum;

/* Allocate a new Bitnum initialized to zero.  */
Bitnum bn_new()
{
}

/* Parse a data stream to extract a Bitnum.  */
Bitnum data_extract()
{
}

/* Return the total size of a Bitnum, in native memory metrics.  */
native_mem bn_size()
{
}

/* Allocate a duplicate of the given Bitnum.  */
Bitnum bn_duplicate()
{
}

int bn_get_bit(Bitnum number, Bitnum index)
{
  /* How do we do this?  */
  /* Parse the header of both `index' and `number'.  */
  /* Make sure that the header of `index' is less than or equal to
     that of `number'.  */
  /* Throw an exception if this is not the case.  */
  /* Loop through both `index' and `number' until the end of `index'
     is reached.  */
  /* Return the indexed bit.  */
}

void bn_set_bit(Bitnum number, Bitnum index, int value)
{
  /* Parse the header of both `index' and `number'.  */
  /* Make sure that the header of `index' is less than or equal to
     that of `number'.  */
  /* Throw an exception if this is not the case.  */
  /* Loop through both `index' and `number' until the end of `index'
     is reached.  */
  /* Set the indexed bit.  */
}

void bn_shl(Bitnum number, Bitnum amount)
{
}

void bn_shr(Bitnum number, Bitnum amount)
{
}

void bn_add(Bitnum result, Bitnum num1, Bitnum num2)
{
}
