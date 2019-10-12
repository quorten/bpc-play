
/* Macintosh monochrome bitmap, most significant bit is #1 on the
   left, least significant bit is right-most.  So we have to handle
   both bit-orderings within a byte.  Simply invert our offset
   calculations by subtracting from 7, and let the user sort out the
   interior ordering.  */

#define GET_BF_CHUNK(pixsize, start, offset, nbytes, bitofs, cptr) \
  nbytes = (pixsize * offset) >> 3; \
  bitofs = (pixsize * offset) & (8 - 1); \
  cptr = (short*)(start + nbytes);

/* TODO: We should provide another macro that advances including a
   previous bit offset.  So here we go.  We add in a pre-filled bit
   offset too for this one.  */
#define GET_BF_P_CHUNK(pixsize, start, offset, nbytes, bitofs, cptr) \
  nbytes = (pixsize * offset + bitofs) >> 3; \
  bitofs = (pixsize * offset + bitofs) & (8 - 1); \
  cptr = (short*)(start + nbytes);

/* Better implementation: */
#define GET_BF_P2_CHUNK(pixsize, start, offset, bitofs, cptr) \
  { \
    unsigned int lgbitofs = pixsize * offset + bitofs; \
    cptr = start + (lgbitofs >> 3); \
    bitofs = lgbitofs & (8 - 1); \
  }

/* Best bet is to require pixsize to be a power of two, which is the
   typical case for packed pixel formats.  Non-power-of-two formats
   typically use padding to be byte-aligned.  */
#define GET_BF_P3_CHUNK(pixbits, start, offset, bitofs, cptr) \
  { \
    unsigned int lgbitofs = (offset << pixbits) + bitofs; \
    cptr = start + (lgbitofs >> 3); \
    bitofs = lgbitofs & (8 - 1); \
  }

/* TODO: And we can implement variants where we only work with a byte
   pointer.  That would generally work better, now that we've split
   and segmented all the code like this.  */

/* Packed bit-wise-data fields.  This is mostly of interest for
   working with packed pixel data, generally all other data use
   structure bit-fields.  */
#define UNPACK_BITFIELD(chunk, bitofs, subofs, finsize, result) \
  result = ((chunk) >> (bitofs + subofs)) & (finsize - 1);

/* Now we have one for pack bitfield.  */
/* Packed bit-wise-data fields.  This is mostly of interest for
   working with packed pixel data, generally all other data use
   structure bit-fields.  */
#define PACK_BITFIELD(chunk, bitofs, subofs, finsize, data) \
  chunk &= ~((finsize - 1) << (bitofs + subofs)); \
  chunk |= (data) << (bitofs + subofs);

/* This works better on CPUs without good shifting instructions.  */
#define NEXT_BF(pixsize, nbytes, bitofs) \
  bitofs += pixsize; \
  if (bitofs >= 8) { \
    bitofs -= 8; \
    nbytes++; \
  }

#define PREV_BF(pixsize, nbytes, bitofs) \
  bitofs -= pixsize; \
  if (bitofs < 0) { \
    bitofs += 8; \
    nbytes--; \
  }

/* Or how about this?  On CPUs with good shifting instructions, this
   is more generalized and works with strides that span more than one
   byte.  Plus, it doesn't require any conditional jumping.  */
#define NEXT_BF(pixsize, nbytes, bitofs) \
  bitofs += pixsize; \
  nbytes += bitofs >> 3; \
  bitofs &= (8 - 1);

/* Now this is tricky, since shifting on negative signed integers is
   not a mathematically correct division by power of two, but hey, for
   our purposes, it works just fine.  */
#define PREV_BF(pixsize, nbytes, bitofs) \
  bitofs -= pixsize; \
  nbytes += bitofs >> 3; \
  bitofs &= (8 - 1);

/* Next scanline?  Add pitch.  Previous scanline?  Subtract pitch.
   Assuming that scanlines are byte aligned, which as I understand,
   was always the case for the main mass market 1980s computer
   systems.  In the case of an esoteric X11 system that did otherwise,
   you have to use modified BF macros.  */

/* Okay, how about a more elegant API.  We provide functions to
   compute the offsets, load a chunk, and we expect the user to save
   the results.  Then we provide functions to read, set, and clear on
   a chunk.  */

/* Now we have efficiency functions for moving to next and previous
   bit-fields.  That should cover it.  Efficiently access a random
   pixel, or efficiently access pixels in order.  */

/* TODO: Important!  Maintain a current pixel position data structure
   and provide optimized functions for single-stepping by a pixel.
   moveto, lineto, lineto... especially is elegant for this.  */

/* Region filling algorithms, two bits per pixel.  Void space, move
   up, move down, straight horizontal.  In the case of zero pixel
   height geometry, we implicitly assume the endpoints where the
   clock-winding flips as an "up" or "down" pixel and flag it as
   such.  */
