/* The boot-time graphics library, it is finally taking shape!  */

/* So, here's how it works.  We define lowest-level primitives that
   know how to work with the pixel data directly.  Then, higher-level
   functions are abstracted and use those primitives.  Of course there
   are always two methods, virtual functions and copy-paste code, the
   second preferred uniformly here since this is performance-critical
   code.  */

/* N.B. A graphical user interface doesn't actually require very many
   graphics drawing primitives to implement.  Basically, a GUI is all
   drawn via bit-block transfers and rectangular region clipping,
   diagonal line drawing is never actually used.  At the very most,
   horizontal and vertical line drawing may be used for drawing window
   and button borders.  Only cache block advancement and row
   advancement pixel iterators are required to support bit-block
   transfers, and often custom row advancement increments will
   computed.  Only vector graphics drawing evokes the full repertoire
   of pixel iterators.  */

/* N.B. Since bit shifting can be expensive or non-existent on some
   microprocessors, we try to avoid bit-shifting in our inner loop
   code as much as possible.  */

/* TODO FIXME: Because of our two-block caching discipline, if the
   image is an add number of cache blocks total, then there must be
   one more cache block padding at the end of the image.  That is, it
   must be possible to read and write one cache block beyond the end
   of the image, though the memory _contents_ will never actually be
   modified.  Just that the existing value will be rewritten.  */

/* TODO FIXME NOTE: My line plotting subroutine?  Actually, thanks to
   its use of iterators, it is fully general and by passing it
   different iterator function pointers, it can plot any function!
   Wow, that's great!  I better get my other function plotters into
   the new code form.  */

/* TODO FIXME: I should probably eliminate the color scanline filler
   and replace it exclusively with a "stipple" pattern filler with a
   special case of pattern equaling cache size.  In order to scan-fill
   an area with a solid color, you must first generate a stipple
   pattern of that color then pass that to the filler routines.  This
   stipple pattern should be cached in the color graphics context,
   generated as soon as you switch the colors.  Of course, in uncached
   mode, stipple pattern fill degenerates into the same case as a
   straight color fill.

   Okay, okay, fine, I digress.  There probably is still some use for
   that primitive for things like Goraud shading.  You might want to
   run a custom calculation function for each pixel, though this is
   obviously slow and expensive.  A specialized ramp filler would be
   more efficient.  Oh, and texture filling.  */

/* TODO FIXME: Image loader format conversion routines?  Those should
   all go in a separate source file together.  Independent of TGA
   format, independent of bootgraph graphics drawing commands, and so
   on.  Because, after all, it is quite common for an application to
   need to do format conversions when loading an image, but it may
   never need to load TGA images or rasterize graphics.  */

/* DEBUG ONLY */
#include <stdio.h>

#include <string.h>

#include "bootgraph.h"

unsigned char g_bg_endian = BG_LE_BITS | BG_LE_BYTES;

/* N.B. This was generated with `bg_print_hdr_bitswap_lut ()`.  */
unsigned char g_bg_bitswap_lut[256] = {
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
  0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
  0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
  0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
  0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
  0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
  0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
  0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
  0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
  0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
  0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
  0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
  0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
  0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
  0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
  0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
  0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
  0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
  0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
  0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
  0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
  0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
  0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
  0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
  0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
  0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
  0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

/* Byte swap */
void
bg_byte_swap (unsigned char *d, unsigned char len)
{
  unsigned char len_div2 = len >> 1;
  unsigned char i;
  for (i = 0; i < len_div2; i++) {
    unsigned char t;
    t = d[i];
    d[i] = d[len-i-1];
    d[len-i-1] = t;
  }
}

/* Bit swap.  Tricky but doable.  The main purpose of implementing
   this function is merely to build the lookup table.  */
unsigned char
bg_bit_swap (unsigned char d)
{
  unsigned char ds = 0;
  unsigned char i;
  for (i = 0; i < 8; i++) {
    ds >>= 1;
    ds |= (d & 0x80);
    d <<= 1;
  }
  return ds;
}

/* Generate the bit swap lookup table.  */
void
bg_gen_bitswap_lut (void)
{
  unsigned short i;
  for (i = 0; i < 256; i++)
    g_bg_bitswap_lut[i] = bg_bit_swap (i);
}

/* Print the generated swap lookup table so that it can be initialized
   within C source code.  */
void
bg_print_hdr_bitswap_lut (void)
{
  unsigned short i;
  puts("unsigned char g_bg_bitswap_lut[256] = {");
  for (i = 0; i < 256; ) {
    unsigned char j;
    fputs(" ", stdout);
    for (j = 0; i < 256 && j < 8; i++, j++) {
      printf(" 0x%02x,", g_bg_bitswap_lut[i]);
    }
    fputs("\n", stdout);
  }
  puts("};");
}

/* Bit swap the given image data.  */
void
bg_bit_swap_image(unsigned char *image_data, unsigned int image_size)
{
  while (image_size > 0) {
    image_size--;
    *image_data = g_bg_bitswap_lut[*image_data];
    image_data++;
  }
}

/* Byte swap each 16-bit word of the given image data.  There must be
   an even number of bytes in the image.  */
void
bg_byte_swap_image16(unsigned char *image_data, unsigned int image_size)
{
  while (image_size > 1) {
    image_size -= 2;
    bg_byte_swap (image_data, 2);
    image_data += 2;
  }
}

/* Byte swap each 24-bit pixel of the given image data.  If there is
   padding between scan lines, you should call this subroutine
   individually for each scanline instead.  */
void
bg_byte_swap_scanln24(unsigned char *image_data, unsigned int image_size)
{
  if ((image_size & (2 - 1)) != 0)
    return;
  while (image_size > 2) {
    image_size -= 3;
    bg_byte_swap (image_data, 3);
    image_data += 3;
  }
}

/* Byte swap each 32-bit word of the given image data.  The image size
   must be a multiple of 4 bytes.  */
void
bg_byte_swap_image32(unsigned char *image_data, unsigned int image_size)
{
  if ((image_size & (4 - 1)) != 0)
    return;
  while (image_size > 0) {
    image_size -= 4;
    bg_byte_swap (image_data, 4);
    image_data += 4;
  }
}

/********************************************************************/

/* Compute the scanline pitch based off of the alignment requirement.
   `align` must be a power of two, max align is 128.  */
unsigned short
bg_align_pitch (unsigned short width, unsigned char align)
{
  unsigned char padding = (align - (unsigned char)width) & (align - 1);
  return width + padding;
}

/* Associate a BgPixIter with an image, and use the desired number of
   bytes for the pixel cache.  If the pixel cache size is less than or
   equal to the size of one pixel, it is effectively disabled.
   Maximum size is 8 bytes.

   `cache_sz_log2` is the base 2 logarithm of the esired cache
   size.  */
void
bg_pit_bind (BgPixIter *pit, RTImageBuf *rti, unsigned char cache_sz_log2,
	     unsigned char twoblk)
{
  unsigned char bpp = rti->hdr.bpp;
  if (cache_sz_log2 > 3)
    cache_sz_log2 = 3;

  memcpy (&pit->rti, rti, sizeof(RTImageBuf));
  pit->pos.x = 0;
  pit->pos.y = 0;
  pit->cblk_addr = 0;
  pit->bit_addr = 0;
  memset (pit->cache, 0, sizeof(pit->cache));
  pit->cache_sz = 1 << cache_sz_log2;
  pit->cache_sz_8 = pit->cache_sz;
  if (twoblk)
    pit->cache_bsz_log2 = cache_sz_log2 - 1;
  else
    pit->cache_bsz_log2 = cache_sz_log2;
  pit->cache_bsz = 1 << pit->cache_bsz_log2;
  pit->cache_bsz_8 = pit->cache_bsz << 3;
  pit->pitch_cblks =
    pit->rti.pitch & ~((unsigned short)pit->cache_bsz - 1);
  pit->pitch_cbits =
    (((unsigned int)pit->rti.pitch << 3) + pit->rti.pitch_bits) &
    (pit->cache_bsz - 1);
  pit->bpp_cblks = (bpp >> 3) & ~(pit->cache_bsz - 1);
  pit->bpp_cbits = bpp & ((pit->cache_bsz << 3) - 1);
  {
    unsigned int row_pad_bits =
      ((unsigned int)pit->rti.pitch << 3) + pit->rti.pitch_bits -
      bpp * pit->rti.hdr.width;
    pit->pitch_pad_cblks =
      (row_pad_bits >> 3) & ~((unsigned short)pit->cache_bsz - 1);
    pit->pitch_pad_cbits = row_pad_bits & ((pit->cache_bsz << 3) - 1);
  }

  /* Uncached mode: 8-bit divisible, cache block divisible bit depths
     can be efficiently accessed without caching.  */
  if ((bpp & (8 - 1)) == 0 &&
      ((bpp >> 3) & (pit->cache_bsz - 1)) == 0) {
    pit->uncached = 1;
    /* Also set `twoblk` to zero since we'll never use the cache
       anyways.  */
    pit->twoblk = 0;
  } else
    pit->uncached = 0;

  pit->twoblk = twoblk;
  pit->valid0 = 0;
  pit->dirty0 = 0;
  pit->valid1 = 0;
  pit->dirty1 = 0;
}

/* Flush the BgPixIter cache, if applicable.

   blk = 1: Flush cache block 0.
   blk = 2: Flush cache block 1.
   blk = 3: Flush both cache blocks.  */
void
bg_pit_flush (BgPixIter *pit, unsigned char blk)
{
  unsigned char cache_bsz;
  if (pit->uncached)
    return;
  cache_bsz = pit->cache_bsz;

  if ((blk & 1) && pit->dirty0) {
    memcpy (pit->rti.image_data + pit->cblk_addr,
	    pit->cache, cache_bsz);
    pit->dirty0 = 0;
  }

  /* Only check for block 1 if we actually are in two-block
     format.  */
  if (pit->twoblk && (blk & 2) && pit->dirty1) {
    memcpy (pit->rti.image_data +
	    pit->cblk_addr + cache_bsz,
	    pit->cache + cache_bsz, cache_bsz);
    pit->dirty1 = 0;
  }
}

/* Flush the BgPixIter cache blocks, if applicable.  */
void
bg_pit_flush_all (BgPixIter *pit)
{
  bg_pit_flush (pit, 3);
}

/* Load the BgPixIter cache to prepare for reading or writing.

   blk = 1: Load cache block 0.
   blk = 2: Load cache block 1.
   blk = 3: Load both cache blocks.  */
void
bg_pit_cload (BgPixIter *pit, unsigned char blk)
{
  unsigned char cache_bsz;
  if (pit->uncached)
    return;
  cache_bsz = pit->cache_bsz;

  if ((blk & 1) && !pit->valid0) {
    memcpy (pit->cache, pit->rti.image_data + 
	    pit->cblk_addr, cache_bsz);
    pit->dirty0 = 0;
    pit->valid0 = 1;
  }

  /* Only check for block 1 if we actually are in two-block
     format.  */
  if (pit->twoblk && (blk & 2) && !pit->valid1) {
    memcpy (pit->cache + cache_bsz,
	    pit->rti.image_data +
	    pit->cblk_addr + cache_bsz, cache_bsz);
    pit->dirty1 = 0;
    pit->valid1 = 1;
  }
}

/* Load the BgPixIter cache blocks, if applicable.  */
void
bg_pit_cload_all (BgPixIter *pit)
{
  bg_pit_cload (pit, 3);
}

/* For two-block mode only: Flush the low memory block and shift.  */
void
bg_pit_twoblk_inc (BgPixIter *pit, unsigned char cblk_inc)
{
  unsigned char cache_bsz;
  if (!pit->twoblk)
    return;
  cache_bsz = pit->cache_bsz;
  bg_pit_flush (pit, 1);
  pit->cblk_addr += cblk_inc;
  /* TODO FIXME: Ideally we would just shift here, but with many
     platforms and compilers we can't be sure if the correct code will
     be generated without a test.  */
  memcpy (pit->cache, pit->cache + cache_bsz, cache_bsz);
  pit->valid0 = pit->valid1;
  pit->dirty0 = pit->dirty1;
  pit->valid1 = 0;
}

/* For two-block mode only: Flush the high memory block and shift.  */
void
bg_pit_twoblk_dec (BgPixIter *pit, unsigned char cblk_dec)
{
  unsigned char cache_bsz;
  if (!pit->twoblk)
    return;
  cache_bsz = pit->cache_bsz;
  bg_pit_flush (pit, 2);
  pit->cblk_addr -= cblk_dec;
  /* TODO FIXME: Ideally we would just shift here, but with many
     platforms and compilers we can't be sure if the correct code will
     be generated without a test.  */
  memcpy (pit->cache + cache_bsz, pit->cache, cache_bsz);
  pit->valid1 = pit->valid0;
  pit->dirty1 = pit->dirty0;
  pit->valid0 = 0;
}

/* Move forward by one cache block.  */
void
bg_pit_cblk_inc (BgPixIter *pit, unsigned char cblk_inc)
{
  if (pit->twoblk)
    /* return */ bg_pit_twoblk_inc (pit, cblk_inc);
  else {
    bg_pit_flush (pit, 1);
    pit->valid0 = 0;
    pit->cblk_addr += cblk_inc;
  }
}

/* Move backward by one cache block.  */
void
bg_pit_cblk_dec (BgPixIter *pit, unsigned char cblk_dec)
{
  if (pit->twoblk)
    /* return */ bg_pit_twoblk_dec (pit, cblk_dec);
  else {
    bg_pit_flush (pit, 1);
    pit->valid0 = 0;
    pit->cblk_addr -= cblk_dec;
  }
}

/* Move the BgPixIter to the given arbitrary coordinate.  Unclipped.
   This is the most computationally expensive movement subroutine, so
   only use it when it is absolutely required.  */
void
bg_pit_moveto (BgPixIter *pit, IPoint2D pt)
{
  unsigned char bpp = pit->rti.hdr.bpp;

  bg_pit_flush_all (pit);
  pit->valid0 = 0;
  pit->valid1 = 0;

  memcpy (&pit->pos, &pt, sizeof(IPoint2D));

  if (pit->uncached) {
    pit->cblk_addr = (unsigned int)pit->pitch_cblks * pt.y +
      (unsigned int)pit->bpp_cblks * pt.x;
  } else {
    /* TODO FIXME: This can be simplified, just use full `bpp` in
       bit_offset computation and remove `bpp_cblks` from here.  */
    unsigned int cblk_offset = (unsigned int)pit->pitch_cblks * pt.y +
      (unsigned int)pit->bpp_cblks * pt.x;
    unsigned int bit_offset = (unsigned int)pit->bpp_cbits * pt.x;
    if (pit->pitch_cbits > 0) {
      bit_offset += (unsigned int)pit->pitch_cbits * pt.y;
    }
    /* N.B. cblk_offset adjustment is only necessary in two cases.
       Assume (bpp == data_bpp + padding_bpp).
       if (pit->bpp_cbits > 0 || pit->pitch_cbits > 0) { then...
    */
    cblk_offset +=
      (bit_offset >> 3) & ~((unsigned int)pit->cache_bsz - 1);
    bit_offset &= (pit->cache_bsz_8 - 1);
    /* ...end } */

    pit->cblk_addr = cblk_offset;
    pit->bit_addr = bit_offset;
  }
}

/* Move the BgPixIter to the given arbitrary coordinate, if possible.
   Clipped.  This is the most computationally expensive movement
   subroutine, so only use it when it is absolutely required.  */
void
bg_pit_moveto_cl (BgPixIter *pit, IPoint2D pt)
{
  if (/* pt.x < 0 || pt.y < 0 || */
      pt.x >= pit->rti.hdr.width || pt.y >= pit->rti.hdr.height)
    return;
  /* return */ bg_pit_moveto (pit, pt);
}

/* TODO FIXME: Add subroutines to advance by n pixels horizontally and
   vertically.  */

/* Positive delta bits, forward by a fraction of a cache block.  */
void
bg_pit_addr_delta_cbits_pu8 (BgPixIter *pit, unsigned char cbits)
{
  unsigned char cache_bsz = pit->cache_bsz;
  unsigned char cache_bsz_8 = pit->cache_bsz_8;
  signed char bit_offset = pit->bit_addr + cbits;
  if (bit_offset >= cache_bsz_8) {
    pit->cblk_addr += cache_bsz;
    bit_offset -= cache_bsz_8;
  }
  pit->bit_addr = bit_offset;
}

/* Negative delta bits, backward by a fraction of a cache block.  */
void
bg_pit_addr_delta_cbits_nu8 (BgPixIter *pit, unsigned char cbits)
{
  unsigned char cache_bsz = pit->cache_bsz;
  unsigned char cache_bsz_8 = pit->cache_bsz_8;
  signed char bit_offset = pit->bit_addr - cbits;
  if (bit_offset < 0) {
    pit->cblk_addr -= cache_bsz;
    bit_offset += cache_bsz_8;
  }
  pit->bit_addr = bit_offset;
}

/* Signed delta bits, forward/backward by a fraction of a cache
   block.  */
void
bg_pit_addr_delta_cbits_i8 (BgPixIter *pit, signed char cbits)
{
  unsigned char cache_bsz = pit->cache_bsz;
  unsigned char cache_bsz_8 = pit->cache_bsz_8;
  signed char bit_offset = pit->bit_addr + cbits;
  if (bit_offset >= cache_bsz_8) {
    pit->cblk_addr += cache_bsz;
    bit_offset -= cache_bsz_8;
  } else if (bit_offset < 0) {
    pit->cblk_addr -= cache_bsz;
    bit_offset += cache_bsz_8;
  }
  pit->bit_addr = bit_offset;
}

void
bg_pit_addr_delta_pu8 (BgPixIter *pit, BGPIAddrDeltaU8 delta)
{
  pit->cblk_addr += delta.cblks;
  if (delta.cbits > 0)
    bg_pit_addr_delta_cbits_pu8 (pit, delta.cbits);
}

void
bg_pit_addr_delta_nu8 (BgPixIter *pit, BGPIAddrDeltaU8 delta)
{
  pit->cblk_addr -= delta.cblks;
  if (delta.cbits > 0)
    bg_pit_addr_delta_cbits_nu8 (pit, delta.cbits);
}

void
bg_pit_addr_delta_i8 (BgPixIter *pit, BGPIAddrDeltaI8 delta)
{
  pit->cblk_addr += (int)delta.cblks;
  if (delta.cbits != 0)
    bg_pit_addr_delta_cbits_i8 (pit, delta.cbits);
}

void
bg_pit_addr_delta_pu16 (BgPixIter *pit, BGPIAddrDeltaU16 delta)
{
  pit->cblk_addr += delta.cblks;
  if (delta.cbits > 0)
    bg_pit_addr_delta_cbits_pu8 (pit, delta.cbits);
}

void
bg_pit_addr_delta_nu16 (BgPixIter *pit, BGPIAddrDeltaU16 delta)
{
  pit->cblk_addr -= delta.cblks;
  if (delta.cbits > 0)
    bg_pit_addr_delta_cbits_nu8 (pit, delta.cbits);
}

void
bg_pit_addr_delta_i16 (BgPixIter *pit, BGPIAddrDeltaI16 delta)
{
  pit->cblk_addr += (int)delta.cblks;
  if (delta.cbits != 0)
    bg_pit_addr_delta_cbits_i8 (pit, delta.cbits);
}

void
bg_pit_addr_delta_pu32 (BgPixIter *pit, BGPIAddrDeltaU32 delta)
{
  pit->cblk_addr += delta.cblks;
  if (delta.cbits > 0)
    bg_pit_addr_delta_cbits_pu8 (pit, delta.cbits);
}

void
bg_pit_addr_delta_nu32 (BgPixIter *pit, BGPIAddrDeltaU32 delta)
{
  pit->cblk_addr -= delta.cblks;
  if (delta.cbits > 0)
    bg_pit_addr_delta_cbits_nu8 (pit, delta.cbits);
}

void
bg_pit_addr_delta_i32 (BgPixIter *pit, BGPIAddrDeltaI32 delta)
{
  pit->cblk_addr += (int)delta.cblks;
  if (delta.cbits != 0)
    bg_pit_addr_delta_cbits_i8 (pit, delta.cbits);
}

/* Convert an "improper fraction" address form to a "proper fraction"
   form and store to the current address.  */
void
bg_pit_addr_proper_u32 (BgPixIter *pit, unsigned int cblks,
			unsigned int cbits)
{
  cblks += (cbits >> 3) & ~((unsigned int)pit->cache_bsz - 1);
  cbits &= (pit->cache_bsz_8 - 1);
  pit->cblk_addr = cblks;
  pit->bit_addr = cbits;
}

/* Move right by one pixel (x + 1), unclipped.  */
void
bg_pit_inc_x (BgPixIter *pit)
{
  pit->pos.x++;

  if (pit->uncached) {
    pit->cblk_addr += pit->bpp_cblks;
  } else {
    /* Compute the incremented address and check if we need to
       flush.  */
    unsigned char bpp_cblks = pit->bpp_cblks;
    unsigned char cache_bsz = pit->cache_bsz;
    unsigned char cache_bsz_8 = pit->cache_bsz_8;
    signed char bit_offset = pit->bit_addr + pit->bpp_cbits;
    if (bit_offset >= cache_bsz_8) {
      bit_offset -= cache_bsz_8;
      bg_pit_cblk_inc (pit, cache_bsz);
    }
    pit->bit_addr = bit_offset;
  }
}

/* Move right by one pixel (x + 1), if possible.  Clipped.  */
void
bg_pit_inc_x_cl (BgPixIter *pit)
{
  if (pit->pos.x == pit->rti.hdr.width - 1)
    return;
  /* return */ bg_pit_inc_x (pit);
}

/* Move left by one pixel (x - 1), unclipped  */
void
bg_pit_dec_x (BgPixIter *pit)
{
  pit->pos.x--;

  if (pit->uncached) {
    pit->cblk_addr -= pit->bpp_cblks;
  } else {
    /* Compute the decremented address and check if we need to
       flush.  */
    unsigned char cache_bsz = pit->cache_bsz;
    unsigned char cache_bsz_8 = pit->cache_bsz_8;
    signed char bit_offset = pit->bit_addr - pit->bpp_cbits;
    if (bit_offset < 0) {
      bit_offset += cache_bsz_8;
      bg_pit_cblk_dec (pit, cache_bsz);
    }
    pit->bit_addr = bit_offset;
  }
}

/* Move left by one pixel (x - 1), if possible.  Clipped.  */
void
bg_pit_dec_x_cl (BgPixIter *pit)
{
  if (pit->pos.x == 0)
    return;
  /* return */ bg_pit_dec_x (pit);
}

/* Move down by one pixel (y + 1), unclipped.  */
void
bg_pit_inc_y (BgPixIter *pit)
{
  pit->pos.y++;

  if (pit->uncached) {
    pit->cblk_addr += pit->pitch_cblks;
  } else {
    BGPIAddrDeltaU16 delta = { pit->pitch_cblks, pit->pitch_cbits };
    /* N.B. When working with very small or narrow images where the
       scanline is narrower than the cache, it may not always be
       necessary to flush the entire cache.  */
    bg_pit_flush_all (pit);
    pit->valid0 = 0;
    pit->valid1 = 0;

    bg_pit_addr_delta_pu16 (pit, delta);
  }
}

/* Move down by one pixel (y + 1), if possible.  Clipped.  */
void
bg_pit_inc_y_cl (BgPixIter *pit)
{
  if (pit->pos.y == pit->rti.hdr.height - 1)
    return;
  /* return */ bg_pit_inc_y (pit);
}

/* Move up by one pixel (y - 1), unclipped.  */
void
bg_pit_dec_y (BgPixIter *pit)
{
  pit->pos.y--;

  if (pit->uncached) {
    pit->cblk_addr -= pit->pitch_cblks;
  } else {
    BGPIAddrDeltaU16 delta = { pit->pitch_cblks, pit->pitch_cbits };
    /* N.B. When working with very small or narrow images where the
       scanline is narrower than the cache, it may not always be
       necessary to flush the entire cache.  */
    bg_pit_flush_all (pit);
    pit->valid0 = 0;
    pit->valid1 = 0;

    bg_pit_addr_delta_nu16 (pit, delta);
  }
}

/* Move up by one pixel (y - 1), if possible.  Clipped.  */
void
bg_pit_dec_y_cl (BgPixIter *pit)
{
  if (pit->pos.y == 0)
    return;
  /* return */ bg_pit_dec_y (pit);
}

/* If we're at the end of a scanline, advance to the beginning of the
   next scanline.  Unclipped.  */
void
bg_pit_next_scanln (BgPixIter *pit)
{
  pit->pos.x = 0;
  pit->pos.y++;

  if (pit->uncached) {
    pit->cblk_addr += pit->pitch_pad_cblks;
  } else {
    BGPIAddrDeltaU16 delta = { pit->pitch_pad_cblks, pit->pitch_pad_cbits };
    /* N.B. When working with very small or narrow images where the
       scanline is narrower than the cache, it may not always be
       necessary to flush the entire cache.  */
    bg_pit_flush_all (pit);
    pit->valid0 = 0;
    pit->valid1 = 0;

    bg_pit_addr_delta_pu16 (pit, delta);
  }
}

/* If we're at the end of a scanline, advance to the beginning of the
   next scanline, if possible.  Clipped.  */
void
bg_pit_next_scanln_cl (BgPixIter *pit)
{
  if (pit->pos.x != pit->rti.hdr.width - 1 ||
      pit->pos.y == pit->rti.hdr.height - 1)
    return;
  /* return */ bg_pit_next_scanln (pit);
}

/* If we're at the beginning of a scanline, advance to the end of the
   previous scanline.  Unclipped.  */
void
bg_pit_prev_scanln (BgPixIter *pit)
{
  pit->pos.x = pit->rti.hdr.width - 1;
  pit->pos.y--;

  if (pit->uncached) {
    pit->cblk_addr -= pit->pitch_pad_cblks;
  } else {
    BGPIAddrDeltaU16 delta = { pit->pitch_pad_cblks, pit->pitch_pad_cbits };
    /* N.B. When working with very small or narrow images where the
       scanline is narrower than the cache, it may not always be
       necessary to flush the entire cache.  */
    bg_pit_flush_all (pit);
    pit->valid0 = 0;
    pit->valid1 = 0;

    bg_pit_addr_delta_nu16 (pit, delta);
  }
}

/* If we're at the beginning of a scanline, advance to the end of the
   previous scanline, if possible.  Clipped.  */
void
bg_pit_prev_scanln_cl (BgPixIter *pit)
{
  if (pit->pos.x != 0 || pit->pos.y == 0)
    return;
  /* return */ bg_pit_next_scanln (pit);
}

/* Compute address delta quantities to advance to the next scanline
   but move forwards/backwards on the x position by the designated
   quantity.  */
void
bg_pit_compute_dix (BgPixIter *pit, BGPIAddrDeltaIX *dx)
{
  if (pit->uncached) {
    dx->delta.cblks =
      pit->pitch_cblks + (short)pit->bpp_cblks * (short)dx->x;
    /* dx->delta.cbits = 0; */
  } else {
    unsigned char cache_bsz = pit->cache_bsz;
    unsigned char cache_bsz_8 = pit->cache_bsz_8;
    /* TODO FIXME: This can be simplified, just use full `bpp` in
       bit_offset computation and remove `bpp_cblks` from here.  */
    short cblks =
      pit->pitch_cblks + (short)pit->bpp_cblks * (short)dx->x;
    int bit_offset = (int)pit->bpp_cbits * (int)dx->x +
      pit->pitch_cbits;
    /* Now, here's the trick.  We use the asymmetric behavior of
       bit-wise operators on signed integers to our advantage.  When
       moving in reverse, we want to round away from zero so we get a
       pointer to the beginning of the block, then AND masking the
       remainder will tell us how many bits forward from that point we
       should be.  */
    cblks += (bit_offset >> 3) & ~((short)cache_bsz - 1);
    dx->delta.cblks = cblks;
    bit_offset &= (cache_bsz_8 - 1);
    dx->delta.cbits = bit_offset;
  }
}

/* Advance to the next scanline and signed delta adjust the x
   position.  Unclipped.  */
void
bg_pit_next_scanln_dix (BgPixIter *pit, BGPIAddrDeltaIX dx)
{
  pit->pos.x += dx.x;
  pit->pos.y++;

  if (pit->uncached) {
    pit->cblk_addr += (int)dx.delta.cblks;
  } else {
    /* N.B. When working with very small or narrow images where the
       scanline is narrower than the cache, it may not always be
       necessary to flush the entire cache.  */
    bg_pit_flush_all (pit);
    pit->valid0 = 0;
    pit->valid1 = 0;

    bg_pit_addr_delta_i16 (pit, dx.delta);
  }
}

/* Advance to the previous scanline and signed delta adjust the x
   position.  Unclipped.  */
void
bg_pit_prev_scanln_dix (BgPixIter *pit, BGPIAddrDeltaIX dx)
{
  pit->pos.x += dx.x;
  pit->pos.y--;

  if (pit->uncached) {
    pit->cblk_addr += (int)dx.delta.cblks;
  } else {
    /* N.B. When working with very small or narrow images where the
       scanline is narrower than the cache, it may not always be
       necessary to flush the entire cache.  */
    bg_pit_flush_all (pit);
    pit->valid0 = 0;
    pit->valid1 = 0;

    bg_pit_addr_delta_i16 (pit, dx.delta);
  }
}

/* Add the unsigned value to the x position.  Unclipped.  */
void
bg_pit_add_x (BgPixIter *pit, unsigned short len)
{
  pit->pos.x += len;

  if (pit->uncached)
    pit->cblk_addr += (unsigned int)pit->bpp_cblks * len;
  else {
    /* If the distance is less than a certain threshold, increment
       multiple times since that will be fastest.  This is quite
       dependent on the performance of the hardware multiply
       instruction, if available.  */
    if (len < 16) {
      while (len > 0) {
	len--;
	bg_pit_inc_x (pit);
      }
    } else {
      /* TODO FIXME: This can be simplified, just use full `bpp` in
	 bit_offset computation and remove `bpp_cblks` from here.  */
      unsigned int cblks = pit->cblk_addr +
	(unsigned int)pit->bpp_cblks * len;
      unsigned int cbits = pit->bit_addr +
	(unsigned int)pit->bpp_cbits * len;

      /* TODO FIXME: This is not always the most efficient cache
	 discipline, but it simplifies our code.  */
      bg_pit_flush_all (pit);
      pit->valid0 = 0;
      pit->valid1 = 0;

      bg_pit_addr_proper_u32 (pit, cblks, cbits);
    }
  }
}

/* Subtract the unsigned value from the x position.  Unclipped.  */
void
bg_pit_sub_x (BgPixIter *pit, unsigned short len)
{
  pit->pos.x -= len;

  if (pit->uncached)
    pit->cblk_addr -= (unsigned int)pit->bpp_cblks * len;
  else {
    /* If the distance is less than a certain threshold, decrement
       multiple times since that will be fastest.  This is quite
       dependent on the performance of the hardware multiply
       instruction, if available.  */
    if (len < 16) {
      while (len > 0) {
	len--;
	bg_pit_dec_x (pit);
      }
    } else {
      /* TODO FIXME: This can be simplified, just use full `bpp` in
	 bit_offset computation and remove `bpp_cblks` from here.  */
      unsigned int cblks = pit->cblk_addr -
	(unsigned int)pit->bpp_cblks * len;
      unsigned int cbits = pit->bit_addr -
	(unsigned int)pit->bpp_cbits * len;

      /* TODO FIXME: This is not always the most efficient cache
	 discipline, but it simplifies our code.  */
      bg_pit_flush_all (pit);
      pit->valid0 = 0;
      pit->valid1 = 0;

      bg_pit_addr_proper_u32 (pit, cblks, cbits);
    }
  }
}

/* Read the bit slice of the given width in bits at the current pixel
   and after, max 64 bits.  */
unsigned long long
bg_pit_read_slice64 (BgPixIter *pit, unsigned char bit_width)
{
  unsigned long long cache_val;
  unsigned long long bw_mask = ((unsigned long long)1 << bit_width) - 1;
  unsigned char cache_sz_8;
  unsigned char cache_bit_ofs;

  /* TODO FIXME: Optimize, don't pre-compute previous quantities
     before this check.  */
  if (pit->uncached) {
    /* In uncached mode, we simply read the memory directly.  */
    unsigned char *addr = &pit->rti.image_data[pit->cblk_addr];
    READ_INTN(cache_val, addr, bit_width);
    return cache_val & bw_mask;
  }

  cache_sz_8 = pit->cache_sz_8;

  /* N.B. Although we could try to selectively determine whether to
     load one or both blocks in two-block mode, it's probably not
     worth the extra computational expense.  */
  bg_pit_cload_all (pit);

  /* Check the byte endian of our host machine and act accordingly.
     Here, we assume the byte endian and the bit endian are the
     same.  */
  cache_bit_ofs = pit->bit_addr;
  if ((g_bg_endian & BG_XE_BYTES) == BG_LE_BYTES)
    ; /* Nothing needs to be done in typical cases.  Hooray!  */
  if ((g_bg_endian & BG_XE_BYTES) == BG_BE_BYTES) {
    /* Gotta do this inversion on big endian or else it won't
       work.  */
    /* TODO FIXME: Check how we can better optimize this, if
       possible.  */
    cache_bit_ofs = cache_sz_8 - cache_bit_ofs - bit_width;
  }
  READ_INTN(cache_val, pit->cache, cache_sz_8);
  return (cache_val >> cache_bit_ofs) & bw_mask;
}

/* Read the color value of the current pixel, max 64 bits per
   pixel.  */
unsigned long long
bg_pit_read_pix64 (BgPixIter *pit)
{
  return bg_pit_read_slice64 (pit, pit->rti.hdr.bpp);
}

/* Read the color value of the current pixel, max 8 bits per
   pixel.  */
unsigned char
bg_pit_read_pix8 (BgPixIter *pit)
{
  return (unsigned char)bg_pit_read_pix64 (pit);
}

/* Read the color value of the current pixel, max 16 bits per
   pixel.  */
unsigned short
bg_pit_read_pix16 (BgPixIter *pit)
{
  return (unsigned short)bg_pit_read_pix64 (pit);
}

/* Read the color value of the current pixel, max 32 bits per
   pixel.  */
unsigned int
bg_pit_read_pix32 (BgPixIter *pit)
{
  return (unsigned int)bg_pit_read_pix64 (pit);
}

/* TODO FIXME: Support other bit-wise write combinators.  AND, OR,
   XOR.  */
/* Write the bit slice of the given width in bits at the current pixel
   and after, max 64 bits.  */
void
bg_pit_write_slice64 (BgPixIter *pit, unsigned long long val,
		      unsigned char bit_width)
{
  unsigned long long bw_mask = ((unsigned long long)1 << bit_width) - 1;
  unsigned char cache_sz_8;
  unsigned char cache_bit_ofs;
  val &= bw_mask;

  if (pit->uncached) {
    /* In uncached mode, we simply write the memory directly.  */
    unsigned char *addr = &pit->rti.image_data[pit->cblk_addr];
    /* TODO FIXME: Try to use a macro here for cleanliness and less
       duplicate code.  */
    switch (bit_width) {
    case 8: *(unsigned char*)addr = val; break;
    case 16: *(unsigned short*)addr = val; break;
    case 32: *(unsigned int*)addr = val; break;
    case 64: *(unsigned long long*)addr = val; break;
    }
    return;
  }

  cache_sz_8 = pit->cache_sz_8;

  /* N.B. Although we could try to selectively determine whether to
     load one or both blocks in two-block mode, it's probably not
     worth the extra computational expense.  */
  bg_pit_cload_all (pit);

  /* TODO FIXME: Duplicate code.  */
  /* Check the byte endian of our host machine and act accordingly.
     Here, we assume the byte endian and the bit endian are the
     same.  */
  cache_bit_ofs = pit->bit_addr;
  if ((g_bg_endian & BG_XE_BYTES) == BG_LE_BYTES)
    ; /* Nothing needs to be done in typical cases.  Hooray!  */
  if ((g_bg_endian & BG_XE_BYTES) == BG_BE_BYTES) {
    /* Gotta do this inversion on big endian or else it won't
       work.  */
    /* TODO FIXME: Check how we can better optimize this, if
       possible.  */
    cache_bit_ofs = cache_sz_8 - cache_bit_ofs - bit_width;
  }

  /* TODO FIXME: Now this is ugly, but there's not really a better way
     to do this in C except with more macros.  The crux is that we
     must not generate unaligned memory requests.  */
  switch (cache_sz_8) {
  case 8:
    *(unsigned char*)pit->cache &=
      ~((unsigned char)bw_mask << cache_bit_ofs);
    *(unsigned char*)pit->cache |=
      (unsigned char)val << cache_bit_ofs;
    break;
  case 16:
    *(unsigned short*)pit->cache &=
      ~((unsigned short)bw_mask << cache_bit_ofs);
    *(unsigned short*)pit->cache |=
      (unsigned short)val << cache_bit_ofs;
    break;
  case 32:
    *(unsigned int*)pit->cache &=
      ~((unsigned int)bw_mask << cache_bit_ofs);
    *(unsigned int*)pit->cache |=
      (unsigned int)val << cache_bit_ofs;
    break;
  case 64:
    *(unsigned long long*)pit->cache &=
      ~((unsigned long long)bw_mask << cache_bit_ofs);
    *(unsigned long long*)pit->cache |=
      (unsigned long long)val << cache_bit_ofs;
    break;
  }

  /* N.B. Although we could try to selectively determine whether to
     dirty-flag one or both blocks in two-block mode, it's probably
     not worth the extra computational expense.  */
  pit->dirty0 = 1;
  pit->dirty1 = 1;
}

/* Write the color value of the current pixel, max 64 bits per
   pixel.  */
void
bg_pit_write_pix64 (BgPixIter *pit, unsigned long long val)
{
  /* return */ bg_pit_write_slice64 (pit, val, pit->rti.hdr.bpp);
}

void
bg_pit_write_pix8 (BgPixIter *pit, unsigned char val)
{
  /* return */ bg_pit_write_pix64 (pit, val);
}

void
bg_pit_write_pix16 (BgPixIter *pit, unsigned short val)
{
  /* return */ bg_pit_write_pix64 (pit, val);
}

void
bg_pit_write_pix32 (BgPixIter *pit, unsigned int val)
{
  /* return */ bg_pit_write_pix64 (pit, val);
}

/* The `get_pix` and `put_pix` functions are provided ONLY for ease of
   programming!  Avoid these functions whenever possible since they
   are inefficient.  */

unsigned long long
bg_pit_get_pix64 (BgPixIter *pit, IPoint2D pt)
{
  bg_pit_moveto (pit, pt);
  return bg_pit_read_pix64 (pit);
}

unsigned char
bg_pit_get_pix8 (BgPixIter *pit, IPoint2D pt)
{
  return (unsigned char)bg_pit_get_pix64 (pit, pt);
}

unsigned short
bg_pit_get_pix16 (BgPixIter *pit, IPoint2D pt)
{
  return (unsigned short)bg_pit_get_pix64 (pit, pt);
}

unsigned int
bg_pit_get_pix32 (BgPixIter *pit, IPoint2D pt)
{
  return (unsigned int)bg_pit_get_pix64 (pit, pt);
}

void
bg_pit_put_pix64 (BgPixIter *pit, IPoint2D pt, unsigned long long val)
{
  bg_pit_moveto (pit, pt);
  /* return */ bg_pit_write_pix64 (pit, val);
}

void
bg_pit_put_pix8 (BgPixIter *pit, IPoint2D pt, unsigned char val)
{
  /* return */ bg_pit_put_pix64 (pit, pt, val);
}

void
bg_pit_put_pix16 (BgPixIter *pit, IPoint2D pt, unsigned short val)
{
  /* return */ bg_pit_put_pix64 (pit, pt, val);
}

void
bg_pit_put_pix32 (BgPixIter *pit, IPoint2D pt, unsigned int val)
{
  /* return */ bg_pit_put_pix64 (pit, pt, val);
}

/********************************************************************/

/* Fill a scanline with the given color value.  The final position is
   one pixel after the last pixel filled.  Unclipped.  */
void
bg_pit_scanline_fill64 (BgPixIter *pit, unsigned short len,
			unsigned long long val)
{
  unsigned short i = len;
  while (i > 0) {
    i--;
    bg_pit_write_pix64 (pit, val);
    bg_pit_inc_x (pit);
  }
}

void
bg_pit_scanline_fill8 (BgPixIter *pit, unsigned short len,
		       unsigned char val)
{
  /* return */ bg_pit_scanline_fill64 (pit, len, val);
}

void
bg_pit_scanline_fill16 (BgPixIter *pit, unsigned short len,
			unsigned short val)
{
  /* return */ bg_pit_scanline_fill64 (pit, len, val);
}

void
bg_pit_scanline_fill32 (BgPixIter *pit, unsigned short len,
			unsigned int val)
{
  /* return */ bg_pit_scanline_fill64 (pit, len, val);
}

/* Reverse fill a scanline with the given color: Starting from the
   current position, count backwards.  The current position is not
   filled, but rather one more backwards position is filled instead.
   The final position is the last pixel filled.  Unclipped.  */
void
bg_pit_scanline_rfill64 (BgPixIter *pit, unsigned short len,
			unsigned long long val)
{
  unsigned short i = len;
  while (i > 0) {
    i--;
    bg_pit_dec_x (pit);
    bg_pit_write_pix64 (pit, val);
  }
}

void
bg_pit_scanline_rfill8 (BgPixIter *pit, unsigned short len,
			unsigned char val)
{
  /* return */ bg_pit_scanline_rfill64 (pit, len, val);
}

void
bg_pit_scanline_rfill16 (BgPixIter *pit, unsigned short len,
			 unsigned short val)
{
  /* return */ bg_pit_scanline_rfill64 (pit, len, val);
}

void
bg_pit_scanline_rfill32 (BgPixIter *pit, unsigned short len,
			 unsigned int val)
{
  /* return */ bg_pit_scanline_rfill64 (pit, len, val);
}

/* Alternate reverse fill a scanline with the given color: Starting
   from the current position, count backwards.  The final position is
   one pixel after the last pixel filled.  Unclipped.  */
void
bg_pit_scanline_arfill64 (BgPixIter *pit, unsigned short len,
			  unsigned long long val)
{
  unsigned short i = len;
  while (i > 0) {
    i--;
    bg_pit_write_pix64 (pit, val);
    bg_pit_dec_x (pit);
  }
}

void
bg_pit_scanline_arfill8 (BgPixIter *pit, unsigned short len,
			 unsigned char val)
{
  /* return */ bg_pit_scanline_arfill64 (pit, len, val);
}

void
bg_pit_scanline_arfill16 (BgPixIter *pit, unsigned short len,
			  unsigned short val)
{
  /* return */ bg_pit_scanline_arfill64 (pit, len, val);
}

void
bg_pit_scanline_arfill32 (BgPixIter *pit, unsigned short len,
			  unsigned int val)
{
  /* return */ bg_pit_scanline_arfill64 (pit, len, val);
}

/* TODO FIXME: Copy scanline, copy with AND + XOR filters.  Pattern
   fill.  Slow versions, fast versions for identical formats, format
   converting versions.

   The best way to do scanline fills, generate a pattern of the
   desired color, then do a pattern fill.  That's the way classic
   Macintosh Quickdraw does it.

   But, actually I can maintain my abstraction primitives and
   performance at the same time.  The main thing I need to support is
   "fat pixels" where I can temporarily treat the graphics operations
   to a wider pixel width data type format for copying multiple pixels
   at the same time.  Simply put, as a higher-level primitive, fetch a
   scanline slice.  The slice size to fetch and store, of course,
   limited by the cache block size.

   Okay, how about this.  So I have my slice functions, but using two
   together would still result in excess shifts.  How about a
   "transfer bit-slice" subroutine?  A little better, but that would
   mean duplicate computation for each scanline.  However, if I define
   "transfer bit-slice with precomputed shift," that works great.  */

/* Clear the image to the given color.  */
void
bg_pit_clear_img64 (BgPixIter *pit, unsigned long long color)
{
  unsigned short width = pit->rti.hdr.width;
  unsigned short height = pit->rti.hdr.height;
  IPoint2D start = { 0, 0 };
  unsigned short i = height;

  bg_pit_moveto (pit, start);
  while (i > 0) {
    i--;
    bg_pit_scanline_fill64 (pit, width, color);
    bg_pit_next_scanln (pit);
  }
}

/* Clear the image to the background color.  */
void
bg_ctx8_clear_img (BgPixIterCol8 *ctx)
{
  BgPixIter *pit = ctx->pit;
  unsigned short width = pit->rti.hdr.width;
  unsigned short height = pit->rti.hdr.height;
  unsigned char bg = ctx->bg;
  IPoint2D start = { 0, 0 };
  unsigned short i = height;

  bg_pit_moveto (pit, start);
  while (i > 0) {
    i--;
    bg_pit_scanline_fill8 (pit, width, bg);
    bg_pit_next_scanln (pit);
  }
}

/********************************************************************/

/* Initialize a BgLineIterY context with 50% rounding.  Unfortunately,
   although 50% rounding looks visually nice, it does come with a
   price of slightly more complex inner loop iterator code.

   Note that although the principal data types are defined as signed
   integers, the algorithms are tuned so that will produce correct
   results with unsigned integers too.  The maximum line length
   supported is half the unsigned integer space.  With signed
   integers, that means the max signed integer value.  */
void
bg_line_iter_y_init (BgLineIterY *lit, Point2D p1, Point2D p2)
{
  Point2D delta = { p2.x - p1.x, p2.y - p1.y };
  Point2D adelta;
  /* TODO FIXME: Check for optimization in sign computation.  */
  Point2D signs;
  unsigned short rem;
  if (delta.x > 0)
    { signs.x = 1; adelta.x = delta.x; }
  else if (delta.x < 0)
    { signs.x = -1; adelta.x = -delta.x; }
  else
    { signs.x = 0; adelta.x = delta.x; }
  if (delta.y > 0)
    { signs.y = 1; adelta.y = delta.y; }
  else if (delta.y < 0)
    { signs.y = -1; adelta.y = -delta.y; }
  else
    { signs.y = 0; adelta.y = delta.y; }
  memcpy (&lit->p1, &p1, sizeof(Point2D));
  memcpy (&lit->p2, &p2, sizeof(Point2D));
  memcpy (&lit->adelta, &adelta, sizeof(Point2D));
  memcpy (&lit->signs, &signs, sizeof(Point2D));
  memcpy (&lit->cur, &p1, sizeof(Point2D));
  /* Initialize the remainder for 50% rounding.  */
  if (adelta.y < adelta.x) {
    lit->neg_start = 1;
    rem = adelta.x - adelta.y;
  } else {
    lit->neg_start = 0;
    rem = adelta.y - adelta.x;
  }
  /* Avoid asymmetric two's complement behavior.  */
  if (rem < 0) rem++;
  rem >>= 1;
  lit->rem = rem;
}

/* `bg_line_iter_y_first ()` is like `bg_line_iter_y_next ()` except
   that it handles the special cases of the first iteration.  */
unsigned char
bg_line_iter_y_first (BgLineIterY *lit)
{
  Point2D adelta;
  Point2D signs;
  Point2D cur;
  short p2_x;
  unsigned short rem;

  /* Copy variables locally for performance.  */
  memcpy (&cur, &lit->cur, sizeof(Point2D));

  /* Check if we are finished.  */
  if (cur.x == lit->p2.x && cur.y == lit->p2.y)
    return 0;

  /* Copy variables locally for performance.  */
  memcpy (&adelta, &lit->adelta, sizeof(Point2D));
  memcpy (&signs, &lit->signs, sizeof(Point2D));
  p2_x = lit->p2.x;
  rem = lit->rem;

  /* On the first iteration, `rem` could be negative.  We do not
     increment the y position in either case, but we start computing
     the initial x position.  This allows us to consistently calculate
     the final y position for (ABS(p2.y - p1.y) + 1) scanlines.  */
  if (lit->neg_start) {
    /* Special case logic to handle the first scanline.  */
    rem = adelta.x - rem;
    while (rem >= adelta.y && cur.x != p2_x) {
      cur.x += signs.x;
      rem -= adelta.y;
    }
  } else {
    rem += adelta.x;
    while (rem >= adelta.y && cur.x != p2_x) {
      cur.x += signs.x;
      rem -= adelta.y;
    }
  }

  /* End of iteration, copy variables back to the context.  */
  memcpy (&lit->cur, &cur, sizeof(Point2D));
  lit->rem = rem;

  return 1;
}

/* `bg_line_iter_y_next ()` steps horizontally until the end of the
   current scanline is exceeded (one pixel beyond the last pixel of a
   scanline).  At this point, by definition, if you drop straight
   up/down, the motion to the next scanline is always a diagonal
   motion, except for straight vertical lines.  You can reference
   `lit->signs.y` for an easy determination of this.

   At the final point, we stop right on the point rather than
   exceeding it.

   So this way, we know both the first and the last pixel on a
   particular scanline, so we can just plug that into higher level
   routines that work on a scanline basis, like triangle and polygon
   filling routines.  */
unsigned char
bg_line_iter_y_next (BgLineIterY *lit)
{
  Point2D adelta;
  Point2D signs;
  Point2D cur;
  short p2_x;
  unsigned short rem;

  /* Copy variables locally for performance.  */
  memcpy (&cur, &lit->cur, sizeof(Point2D));

  /* Check if we are finished.  */
  if (cur.x == lit->p2.x && cur.y == lit->p2.y)
    return 0;

  /* Copy variables locally for performance.  */
  memcpy (&adelta, &lit->adelta, sizeof(Point2D));
  memcpy (&signs, &lit->signs, sizeof(Point2D));
  p2_x = lit->p2.x;
  rem = lit->rem;

  rem += adelta.x;
  cur.y += signs.y;
  while (rem >= adelta.y && cur.x != p2_x) {
    cur.x += signs.x;
    rem -= adelta.y;
  }

  /* End of iteration, copy variables back to the context.  */
  memcpy (&lit->cur, &cur, sizeof(Point2D));
  lit->rem = rem;

  return 1;
}

/* Draw a line from the current position up to, but not including, the
   given point.  Unclipped.  */
void
bg_pit_lineto64 (BgPixIter *pit, IPoint2D p2, unsigned long long val)
{
  BgLineIterY lit;
  IPoint2D last_pt = { pit->pos.x, pit->pos.y };
  /* TODO FIXME: We should revamp our iterator use here so we get an
     iterator that directly returns the length of each scanline slice,
     rather than the positions.  */
  bg_line_iter_y_init (&lit, *(Point2D*)&last_pt, *(Point2D*)&p2);

  /* Now, this may look quite funny, but fortunately we can use this
     setup that is similar to a "Duff Device" for the sake of maximum
     code sharing when working with iterator code.  Because of the
     "non-local goto," it's important that we do not declare any
     variables at the top of the while loop scope since we will
     literally skip over the machine code that allocates such
     storage.  */

  /* First iteration */
  if (!bg_line_iter_y_first (&lit))
    return; /* Nothing to be done.  */
  goto common;

  /* All next iterations */
  while (bg_line_iter_y_next (&lit)) {
    /* Advance the y position accordingly.  */
    switch (lit.signs.y) {
    case 1: bg_pit_inc_y (pit); break;
    case -1: bg_pit_dec_y (pit); break;
    }
  common:
    /* The iterator computed the first point on the next scanline, so
       now we can compute the fill scanline parameters.  */
    if (lit.cur.x >= last_pt.x) {
      unsigned short len = lit.cur.x - last_pt.x;
      if (len == 0)
	bg_pit_write_pix64 (pit, val);
      else
	bg_pit_scanline_fill64 (pit, len, val);
    } else {
      /* Adjust the fill parameters so that we include the first point
	 but not the last point when stepping backwards.  */
      unsigned short len = last_pt.x - lit.cur.x;
      bg_pit_scanline_arfill64 (pit, len, val);
    }
    /* Update `last_pt`.  */
    last_pt.x = lit.cur.x;
    last_pt.y = lit.cur.y;
  }
}

/* Specialized triangle outline drawer, suitable for rasterization of
   3D graphics.  Unclipped.  */
void
bg_pit_tri_line64 (BgPixIter *pit,
		   IPoint2D p1, IPoint2D p2, IPoint2D p3,
		   unsigned long long val)
{
  bg_pit_moveto (pit, p1);
  bg_pit_lineto64 (pit, p2, val);
  bg_pit_lineto64 (pit, p3, val);
  bg_pit_lineto64 (pit, p1, val);
}

/* Specialized quadrilateral outline drawer, suitable for
   rasterization of 3D graphics.  Unclipped.  */
void
bg_pit_quad_line64 (BgPixIter *pit,
		    IPoint2D p1, IPoint2D p2, IPoint2D p3, IPoint2D p4,
		    unsigned long long val)
{
  bg_pit_moveto (pit, p1);
  bg_pit_lineto64 (pit, p2, val);
  bg_pit_lineto64 (pit, p3, val);
  bg_pit_lineto64 (pit, p4, val);
  bg_pit_lineto64 (pit, p1, val);
}

/* Specialized triangle filler, suitable for rasterization of 3D
   graphics.  Unclipped.

   Fill rule: the first scanline (ymin) is filled but the last
   scanline (ymax) is not.  Likewise, the first x pixel (xmin) is
   filled but the last one (xmax) is not.  */
void
bg_pit_tri_fill64 (BgPixIter *pit,
		   IPoint2D p1, IPoint2D p2, IPoint2D p3,
		   unsigned long long val)
{
  BgLineIterY lit1, lit2;
  IPoint2D last_pt1, last_pt2;
  unsigned char i;
  /* In order to conveniently use bit-fields to pack together multiple
     boolean variables, we can define an anonymous structure.  */
  struct
  {
    unsigned char x_reverse : 1;
    unsigned char zigzag_left : 1;
  } f;
  f.zigzag_left = 0;

  /* First sort the points in vertical ascending order.  */
  if (p2.y < p1.y) SWAP_PTS(p1, p2);
  if (p3.y < p2.y) SWAP_PTS(p2, p3);
  if (p2.y < p1.y) SWAP_PTS(p1, p2);

  /* Check if there are actually zero scanlines to render and exit
     early.  */
  if (p3.y == p1.y)
    return;

  bg_pit_moveto (pit, p1);
  last_pt1.x = p1.x; last_pt1.y = p1.y;
  last_pt2.x = p1.x; last_pt2.y = p1.y;
  bg_line_iter_y_init (&lit2, *(Point2D*)&p1, *(Point2D*)&p3);

  i = 2;
  do {
    unsigned short *begin_x, *end_x;
    unsigned short len;

    i--;
    if (i == 1) {
      /* Scan-fill the triangle between lines from first vertex, until
	 we reach the height of the second vertex.  */
      bg_line_iter_y_init (&lit1, *(Point2D*)&p1, *(Point2D*)&p2);
      if ((p2.x == p3.x && p2.y > p3.y) ||
	  p2.x > p3.x)
	f.x_reverse = 1;
      else
	f.x_reverse = 0;

      /* N.B.: Pointers are used here as a code efficiency compromise
	 between to eliminate extra branches in inner loop, at the
	 expense of additional pointer dereferencing in the loop.
	 That can create trouble with register caching of the values.
	 Short of that, the only other way to get more efficiency is
	 to copy-paste the inner loop code 8 times for each
	 possibility.  */
      if (f.x_reverse) {
	begin_x = (lit2.signs.x > 0) ?
	  &last_pt2.x : (unsigned short*)&lit2.cur.x;
	end_x = (lit1.signs.x > 0) ?
	  &last_pt1.x : (unsigned short*)&lit1.cur.x;
      } else {
	begin_x = (lit1.signs.x > 0) ?
	  &last_pt1.x : (unsigned short*)&lit1.cur.x;
	end_x = (lit2.signs.x > 0) ?
	  &last_pt2.x : (unsigned short*)&lit2.cur.x;
      }

      /* Use the same "Duff Device" trick here that we used in the
	 line rasterizer.  */
      if (!bg_line_iter_y_first (&lit1))
	continue; /* Nothing to be done.  */
      bg_line_iter_y_first (&lit2);
      /* goto common; comes next */
    } else {
      /* Scan-fill the triangle between lines to the last vertex,
	 until we reach the last vertex.  */
      bg_line_iter_y_init (&lit1, *(Point2D*)&p2, *(Point2D*)&p3);
      /* N.B.: `f.x_reverse` will be the same as the first pair, so we
	 do not need to recompute it.  */

      /* Use the same "Duff Device" trick here that we used in the
	 line rasterizer.  */
      if (!bg_line_iter_y_first (&lit1))
	continue; /* Nothing to be done.  */
      bg_line_iter_y_next (&lit2);
      /* goto common; comes next */
    }

    goto common;

    while (bg_line_iter_y_next (&lit1)) {
      bg_line_iter_y_next (&lit2);
    common:
      /* Verify we do not fill the very last scanline.  */
      if (lit2.cur.y == p3.y)
	continue;
      /* The iterators computed the first point on the next scanline, so
	 now we can compute the fill scanline parameters.  */
      if (*end_x < *begin_x)
	len = 0; /* ???  It can happen in tight corners.  */
      else
	len = *end_x - *begin_x;
      /* TODO FIXME: Should we avoid zigzag scanfill?  If we wanted to
	 avoid it, we'd have to compute the triangular next scanline
	 skip factor.  */
      if (f.zigzag_left) {
	/* TODO FIXME: Shift into position for the scanline endpoint,
	   is this inefficient?  */
	if (*end_x >= pit->pos.x) {
	  unsigned short i = *end_x - pit->pos.x;
	  while (i > 0) {
	    i--;
	    bg_pit_inc_x (pit);
	  }
	} else {
	  unsigned short i = pit->pos.x - *end_x;
	  while (i > 0) {
	    i--;
	    bg_pit_dec_x (pit);
	  }
	}
	bg_pit_scanline_rfill64 (pit, len, val);
	f.zigzag_left = 0;
      } else {
	/* TODO FIXME: Shift into position for the scanline beginning,
	   is this inefficient?  */
	if (*begin_x >= pit->pos.x)
	  bg_pit_add_x (pit, *begin_x - pit->pos.x);
	else
	  bg_pit_sub_x (pit, pit->pos.x - *begin_x);
	bg_pit_scanline_fill64 (pit, len, val);
	f.zigzag_left = 1;
      }
      /*{
	IPoint2D mpt1 = { *begin_x, last_pt2.y };
	bg_pit_moveto (pit, mpt1);
	bg_pit_scanline_fill64 (pit, len, val);
      }*/
      /* Advance the y position accordingly.  */
      bg_pit_inc_y (pit);
      /* Update `last_pt1` and `last_pt2`.  */
      last_pt1.x = lit1.cur.x; last_pt1.y = lit1.cur.y;
      last_pt2.x = lit2.cur.x; last_pt2.y = lit2.cur.y;
    }
  } while (i > 0);
}

/********************************************************************/

/*

Pixel format and color space conversions.

So, how does this work?  We rely on these principal definitions.

* Default color pixel format, three-channel: red, green, blue

* Optional fourth channel: alpha

* Rare fourth channel: Infrared

* Rare "bird vision" fourth channel: Ultraviolet.

* Two channel format?  Extremely rare, the only somewhat notable
  two-channel format is red-green Technicolor.

* The de facto one channel format is grayscale.

* 16 bits per channel is the principal linear intensity channel
  format.

* 8 bits per channel is a gamma-coded compression of the 16-bit linear
  format.  An 8-bit alpha channel, however, could also as a linear
  format.  The principal gamma-coding is either sRGB or Gamma = 2.2.

* 5 bits or 6 bits per channel (i.e. 16-bit high color R5G6R5) is
  simply the same as 8-bit gamma-coded image samples, but with the
  least significant bits truncated to zero.

* 8 bit _per pixel_ or less are all to be interpreted as palette-based
  formats.  Each pixel is an index into the palette, and a palette
  entry defines a color more precisely in one of the three-channel
  formats, typically 8 bits per channel gamma-coded image samples.

  The most straightforward 8-bits per pixel format is grayscale, where
  red, green, and blue all share the same intensity.

* 4, 3, and 1 bit per pixel palettes have only a few common and rather
  trivial palettes that are typically used.

Finally, the remainder of information that defines a pixel format is
the bit and byte ordering.  With less than 8 bits per pixel, there is
ia "bit endian" ordering on each pixel.  Leftmost first pixel as least
significant bits ("little endian" bits), or leftmost first pixel as
most significant bits ("big endian" bits).  Index and channel bit
endian, however, is almost always unambiguous: whatever the logical
bit ordering for integers is.  Finally, once you get to multi-byte
intensities, byte order becomes significant, but not bit order.

So, bit order:

* "Logical", standard integer bit ordering, only applicable for
  integer-like values
* Little Endian
* Big Endian

Byte order:

* Little Endian
* Big Endian

In practice, we only define data structure storage for these of the
machine CPU attributes itself.  For images, specific format loaders
will simply call the conversion functions with the appropriate
parameters.

*/

/* Monochrome color palette.  Zero is black, one is white.  */
RgbPix g_stdpal_1bit[2] = {
  { 0x00, 0x00, 0x00 },
  { 0xff, 0xff, 0xff },
};

/* Macintosh monochrome color palette.  Zero is white, one is
   black.  */
RgbPix g_stdpal_mac_1bit[2] = {
  { 0xff, 0xff, 0xff },
  { 0x00, 0x00, 0x00 },
};

/* Rumored 3-bit Macintosh color.  I assume the palette indices are
   arranged in RGB bit field orders with the most significant bit
   corresponding to red and the least significant bit corresponding to
   blue.  */
RgbPix g_stdpal_mac_3bit[8] = {
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0xff },
  { 0x00, 0xff, 0x00 },
  { 0x00, 0xff, 0xff },
  { 0xff, 0x00, 0x00 },
  { 0xff, 0x00, 0xff },
  { 0xff, 0xff, 0x00 },
  { 0xff, 0xff, 0xff },
};

/* The famous 4-bit VGA Windows color palette.  */
RgbPix g_stdpal_vga_4bit[16] = {
  { 0x00, 0x00, 0x00 },
  { 0x80, 0x00, 0x00 },
  { 0x00, 0x80, 0x00 },
  { 0x80, 0x80, 0x00 },
  { 0x00, 0x00, 0x80 },
  { 0x80, 0x00, 0x80 },
  { 0x00, 0x80, 0x80 },
  { 0x80, 0x80, 0x80 },
  { 0xc0, 0xc0, 0xc0 },
  { 0xff, 0x00, 0x00 },
  { 0x00, 0xff, 0x00 },
  { 0xff, 0xff, 0x00 },
  { 0x00, 0x00, 0xff },
  { 0xff, 0x00, 0xff },
  { 0x00, 0xff, 0xff },
  { 0xff, 0xff, 0xff },
};
