#ifndef BOOTGRAPH_H
#define BOOTGRAPH_H

struct Point2D_tag
{
  short x;
  short y;
};
typedef struct Point2D_tag Point2D;

/* Image point, i.e. corresponds to a memory location.  */
struct IPoint2D_tag
{
  unsigned short x;
  unsigned short y;
};
typedef struct IPoint2D_tag IPoint2D;

/* Note that this is actually a compatible subset of a full Truevision
   TGA TARGA header.  */
struct ImageBufHdr_tag
{
  unsigned short width;
  unsigned short height;
  unsigned char bpp;
  /* Image flags, most important are orientation.  */
  unsigned char image_desc;
};
typedef struct ImageBufHdr_tag ImageBufHdr;

/* Pixel data transfer from file to screen:
   These masks are AND'd with the imageDesc in the TGA header,
   bit 4 is left-to-right ordering
   bit 5 is top-to-bottom */
#define BOTTOM_LEFT  0x00	/* first pixel is bottom left corner */
#define BOTTOM_RIGHT 0x10	/* first pixel is bottom right corner */
#define TOP_LEFT     0x20	/* first pixel is top left corner */
#define TOP_RIGHT    0x30	/* first pixel is top right corner */

/* Little Endian, least significant bit/byte is first pixel */
#define BG_LE_BITS 0
#define BG_LE_BYTES 0
/* Big Endian, most significant bit/byte is first pixel */
#define BG_BE_BITS 1
#define BG_BE_BYTES 2
/* X-Endian test masks */
#define BG_XE_BITS 1
#define BG_XE_BYTES 2

/* PLEASE NOTE: Our "runtime image buffer" is designed with the
   assumption that the bit endian and byte endian are pre-converted to
   the format that is most efficient to compute with on the host
   machine.  This makes the most logical sense from a performance
   standpoint when you will be computing directly on the image using
   your own CPU.  Therefore, when our code checks the endian, we only
   check the machine's byte endian, and we assume the bit and byte
   endian are the same.  */

/* Runtime image buffer.  */
struct RTImageBuf_tag
{
  unsigned char *image_data;
  unsigned int image_size;
  unsigned short pitch; /* size in bytes of one scanline */
  /* For non-byte aligned scanlines, the additional size in bits of
     the scanline.  */
  unsigned char pitch_bits;
  ImageBufHdr hdr;
};
typedef struct RTImageBuf_tag RTImageBuf;

#define ABS(x) (((x) >= 0) ? (x) : -(x))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/* The clever XOR swap that somewhat famously didn't work on the Alpha
   CPU version of Windows NT due to a compiler bug, under some
   circumstances.  Specifically, if you were trying to use it to swap
   points.  Use at your own risk, this is probably generally less
   efficient than just using a temporary variable.  After all, an
   optimizing compiler should be able to determine that you're doing a
   swap and switch to the alternate form to save registers and memory
   access if needed.  */
#define XOR_SWAP(a, b) ((a) ^= (b) ^= (a) ^= (b))

/* Nevertheless, XOR swapping is a fairly common method of performing
   byte swaps.  */

#define SWAP_PTS(p1, p2) \
  { \
    IPoint2D temp = { p1.x, p1.y }; \
    p1.x = p2.x; p1.y = p2.y; \
    p2.x = temp.x; p2.y = temp.y; \
  }

/* This would be it, the buggy code.  */
/* #define SWAP_PTS(p1, p2) (XOR_SWAP(p1.x, p2.x), XOR_SWAP(p1.y, p2.y)) */

/* Read an integer of `size` in bytes at `ptr` using C-style casting
   and store the result in `result`.  */
#define READ_INTN(result, ptr, size) \
  switch (size) { \
  case 8: (result) = *(unsigned char*)(ptr); break; \
  case 16: (result) = *(unsigned short*)(ptr); break; \
  case 32: (result) = *(unsigned int*)(ptr); break; \
  case 64: (result) = *(unsigned long long*)(ptr); break; \
  default: (result) = 0; break; \
  }

/* Quotient-remainder mathematical data structures.  "Proper
   fractions" in other words.  The dividend itself is assumed to be
   known based off of the data type, so this is technically a sort of
   abstract base class.

   TODO: Chains of proper fractions are very useful for units and also
   for Apple II display framebuffers.  Miles, yards, feet, inches, for
   example.  */
struct IVQuotRem_u8_divu8_tag
{
  unsigned char w; /* Quotient, whole part */
  unsigned char n; /* Remainder, numerator */
};
struct IVQuotRem_u8_divu8_tag IVQuotRem_u8_divu8;

struct IVQuotRem_u16_divu8_tag
{
  unsigned short w; /* Quotient, whole part */
  unsigned char n; /* Remainder, numerator */
};
struct IVQuotRem_u8_divu8_tag IVQuotRem_u8_divu8;

/* The pixel iterator, the base object of all graphics drawing
   operations.  A block subset of a scanline's pixels can be cached
   when the size of a pixel is smaller than the minimum supported
   memory block, or simply to accelerate performance.  A pixel
   generally should not be larger than a cache block, though proper
   alignment in two-block mode can permit the use of larger pixels.
   Routines are provided for moving up and down by one pixel with
   accelerated address computation.

   The cache can be split into two independently managed "blocks."
   This allows us to work with pixels that span the memory access
   alignment boundary while keeping our memory accesses still
   aligned.  */
struct BgPixIter_tag
{
  /* N.B.: For performance, we copy the whole RTImageBuf data
     structure into the BgPixIter.  This means that if you make any
     changes to the source data, you must re-bind the BgPixIter to the
     RTImageBuf.  */
  RTImageBuf rti;
  unsigned char cache[8]; /* pixel cache */
  IPoint2D pos;
  /* Offset from base address, in bytes, to the address of the cached
     block.  */
  unsigned int cblk_addr;
  /* Image `pitch` (row size in bytes), but only the byte count that
     includes a whole number of cache blocks `cache_bsz`.  */
  unsigned short pitch_cblks;
  /* Padding at the end of scanlines to reach the indicated `pitch`
     size.  */
  unsigned short pitch_pad_cblks;
  /* Remainder bits of `pitch` that do not fit within `cache_bsz`.  */
  unsigned char pitch_cbits;
  unsigned char pitch_pad_cbits;
  /* Number of bytes of whole cache blocks `cache_bsz` that fit in one
     pixel's memory.  */
  unsigned char bpp_cblks;
  /* Remainder of bits of a pixel that don't fit within
     `cache_bsz`.  */
  unsigned char bpp_cbits;
  /* Offset in bits from the cached block to the current pixel.  */
  unsigned char bit_addr;
  /* User-adjustable total pixel cache size in bytes, up to the
     maximum of 8.  */
  unsigned char cache_sz;
  /* `cache_sz * 8` */
  unsigned char cache_sz_8;
  /* Size of a single cache block in bytes, half the total cache size
     in two-block mode.  */
  unsigned char cache_bsz;
  /* `cache_bsz * 8` */
  unsigned char cache_bsz_8;
  /* Base 2 logarithm of `cache_bsz`.  */
  unsigned char cache_bsz_log2;
  /* Uncached mode, automatically determined based off of
     compatibility.  */
  unsigned char uncached : 1;
  /* Should the cache be treated as two blocks rather than one?  */
  unsigned char twoblk : 1;
  /* Is the cache block 0 valid for reading?  */
  unsigned char valid0 : 1;
  /* Do we have an unflushed cache block 0?  */
  unsigned char dirty0 : 1;
  /* Is the cache block 1 valid for reading?  */
  unsigned char valid1 : 1;
  /* Do we have an unflushed cache block 1?  */
  unsigned char dirty1 : 1;
};
typedef struct BgPixIter_tag BgPixIter;

/* Independent pixel iterator address structure, used for saving and
   restoring a particular address.  */
struct BgPixIterAddr_tag
{
  IPoint2D pos;
  unsigned int cblk_addr;
  unsigned char bit_addr;
};
typedef struct BgPixIterAddr_tag BgPixIterAddr;

/* Precomputed address advancement deltas.  */
struct BGPIAddrDeltaU8_tag
{
  unsigned char cblks;
  unsigned char cbits;
};
typedef struct BGPIAddrDeltaU8_tag BGPIAddrDeltaU8;

struct BGPIAddrDeltaI8_tag
{
  signed char cblks;
  signed char cbits;
};
typedef struct BGPIAddrDeltaI8_tag BGPIAddrDeltaI8;

struct BGPIAddrDeltaU16_tag
{
  unsigned short cblks;
  unsigned char cbits;
};
typedef struct BGPIAddrDeltaU16_tag BGPIAddrDeltaU16;

struct BGPIAddrDeltaI16_tag
{
  short cblks;
  signed char cbits;
};
typedef struct BGPIAddrDeltaI16_tag BGPIAddrDeltaI16;

struct BGPIAddrDeltaU32_tag
{
  unsigned int cblks;
  unsigned char cbits;
};
typedef struct BGPIAddrDeltaU32_tag BGPIAddrDeltaU32;

struct BGPIAddrDeltaI32_tag
{
  int cblks;
  signed char cbits;
};
typedef struct BGPIAddrDeltaI32_tag BGPIAddrDeltaI32;

struct BGPIAddrDeltaIX_tag
{
  short x;
  BGPIAddrDeltaI16 delta;
};
typedef struct BGPIAddrDeltaIX_tag BGPIAddrDeltaIX;

struct BGPIAddrDeltaIY_tag
{
  short y;
  BGPIAddrDeltaI32 delta;
};
typedef struct BGPIAddrDeltaIY_tag BGPIAddrDeltaIY;

struct BGPIAddrDeltaIXY_tag
{
  Point2D pos;
  BGPIAddrDeltaI32 delta;
};
typedef struct BGPIAddrDeltaIXY_tag BGPIAddrDeltaIXY;

/* Position and 8-bit color context, very useful for many basic
   drawing operations that need only know of a foreground and
   background color.  */
struct BgPixIterCol8_tag
{
  BgPixIter *pit;
  unsigned char bg; /* background color */
  unsigned char fg; /* foreground color */
};
typedef struct BgPixIterCol8_tag BgPixIterCol8;

/* Position and 16-bit color context.  */
struct BgPixIterCol16_tag
{
  BgPixIter *pit;
  unsigned short bg; /* background color */
  unsigned short fg; /* foreground color */
};
typedef struct BgPixIterCol16_tag BgPixIterCol16;

/* Position and 32-bit color context.  */
struct BgPixIterCol32_tag
{
  BgPixIter *pit;
  unsigned int bg; /* background color */
  unsigned int fg; /* foreground color */
};
typedef struct BgPixIterCol32_tag BgPixIterCol32;

/* Position and 64-bit color context.  */
struct BgPixIterCol64_tag
{
  BgPixIter *pit;
  unsigned long long bg; /* background color */
  unsigned long long fg; /* foreground color */
};
typedef struct BgPixIterCol64_tag BgPixIterCol64;

/* Line iterator, major y-axis.  Basically, this is a pixel stepping
   iterator using Bresenham's line plotting algorithm, designed to be
   general mathematical primitive so that it is not confined to 2D
   graphics line plotting.  */
struct BgLineIterY_tag
{
  Point2D p1;
  Point2D p2;
  Point2D adelta;
  Point2D signs;
  Point2D cur;
  unsigned short rem;
  /* Is the remainder initialized to the absolute value of a negative
     value?  */
  unsigned char neg_start;
};
typedef struct BgLineIterY_tag BgLineIterY;

struct RgbPix_tag
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
};
typedef struct RgbPix_tag RgbPix;

extern unsigned char g_bg_endian;
extern unsigned char g_bg_bitswap_lut[256];

void bg_byte_swap (unsigned char *d, unsigned char len);
unsigned char bg_bit_swap (unsigned char d);
void bg_gen_bitswap_lut (void);
void bg_print_hdr_bitswap_lut (void);
void bg_bit_swap_image(unsigned char *image_data, unsigned int image_size);
void bg_byte_swap_image16(unsigned char *image_data, unsigned int image_size);
void bg_byte_swap_scanln24(unsigned char *image_data, unsigned int image_size);
void bg_byte_swap_image32(unsigned char *image_data, unsigned int image_size);

unsigned short bg_align_pitch (unsigned short width, unsigned char align);
void bg_pit_bind (BgPixIter *pit, RTImageBuf *rti,
		  unsigned char cache_sz_log2, unsigned char twoblk);
void bg_pit_flush (BgPixIter *pit, unsigned char blk);
void bg_pit_flush_all (BgPixIter *pit);
void bg_pit_cload (BgPixIter *pit, unsigned char blk);
void bg_pit_cload_all (BgPixIter *pit);
void bg_pit_twoblk_inc (BgPixIter *pit, unsigned char cblk_inc);
void bg_pit_twoblk_dec (BgPixIter *pit, unsigned char cblk_dec);
void bg_pit_cblk_inc (BgPixIter *pit, unsigned char cblk_inc);
void bg_pit_cblk_dec (BgPixIter *pit, unsigned char cblk_dec);
void bg_pit_moveto (BgPixIter *pit, IPoint2D pt);
void bg_pit_moveto_cl (BgPixIter *pit, IPoint2D pt);
void bg_pit_addr_delta_cbits_pu8(BgPixIter *pit, unsigned char cbits);
void bg_pit_addr_delta_cbits_nu8(BgPixIter *pit, unsigned char cbits);
void bg_pit_addr_delta_cbits_i8(BgPixIter *pit, signed char cbits);
void bg_pit_addr_delta_pu8(BgPixIter *pit, BGPIAddrDeltaU8 delta);
void bg_pit_addr_delta_nu8(BgPixIter *pit, BGPIAddrDeltaU8 delta);
void bg_pit_addr_delta_i8(BgPixIter *pit, BGPIAddrDeltaI8 delta);
void bg_pit_addr_delta_pu16(BgPixIter *pit, BGPIAddrDeltaU16 delta);
void bg_pit_addr_delta_nu16(BgPixIter *pit, BGPIAddrDeltaU16 delta);
void bg_pit_addr_delta_i16(BgPixIter *pit, BGPIAddrDeltaI16 delta);
void bg_pit_addr_delta_pu32(BgPixIter *pit, BGPIAddrDeltaU32 delta);
void bg_pit_addr_delta_nu32(BgPixIter *pit, BGPIAddrDeltaU32 delta);
void bg_pit_addr_delta_i32(BgPixIter *pit, BGPIAddrDeltaI32 delta);
void bg_pit_addr_proper_u32 (BgPixIter *pit, unsigned int cblks,
			     unsigned int cbits);
void bg_pit_inc_x (BgPixIter *pit);
void bg_pit_inc_x_cl (BgPixIter *pit);
void bg_pit_dec_x (BgPixIter *pit);
void bg_pit_dec_x_cl (BgPixIter *pit);
void bg_pit_inc_y (BgPixIter *pit);
void bg_pit_inc_y_cl (BgPixIter *pit);
void bg_pit_dec_y (BgPixIter *pit);
void bg_pit_dec_y_cl (BgPixIter *pit);
void bg_pit_next_scanln (BgPixIter *pit);
void bg_pit_next_scanln_cl (BgPixIter *pit);
void bg_pit_prev_scanln (BgPixIter *pit);
void bg_pit_prev_scanln_cl (BgPixIter *pit);
void bg_pit_compute_dix (BgPixIter *pit, BGPIAddrDeltaIX *dx);
void bg_pit_next_scanln_dix (BgPixIter *pit, BGPIAddrDeltaIX dx);
void bg_pit_prev_scanln_dix (BgPixIter *pit, BGPIAddrDeltaIX dx);
void bg_pit_add_x (BgPixIter *pit, unsigned short len);
void bg_pit_sub_x (BgPixIter *pit, unsigned short len);
unsigned long long bg_pit_read_slice64 (BgPixIter *pit,
					unsigned char bit_width);
unsigned long long bg_pit_read_pix64 (BgPixIter *pit);
unsigned char bg_pit_read_pix8 (BgPixIter *pit);
unsigned short bg_pit_read_pix16 (BgPixIter *pit);
unsigned int bg_pit_read_pix32 (BgPixIter *pit);
void bg_pit_write_slice64 (BgPixIter *pit, unsigned long long val,
			   unsigned char bit_width);
void bg_pit_write_pix64 (BgPixIter *pit, unsigned long long val);
void bg_pit_write_pix8 (BgPixIter *pit, unsigned char val);
void bg_pit_write_pix16 (BgPixIter *pit, unsigned short val);
void bg_pit_write_pix32 (BgPixIter *pit, unsigned int val);
unsigned long long bg_pit_get_pix64 (BgPixIter *pit, IPoint2D pt);
unsigned char bg_pit_get_pix8 (BgPixIter *pit, IPoint2D pt);
unsigned short bg_pit_get_pix16 (BgPixIter *pit, IPoint2D pt);
unsigned int bg_pit_get_pix32 (BgPixIter *pit, IPoint2D pt);
void bg_pit_put_pix64 (BgPixIter *pit, IPoint2D pt, unsigned long long val);
void bg_pit_put_pix8 (BgPixIter *pit, IPoint2D pt, unsigned char val);
void bg_pit_put_pix16 (BgPixIter *pit, IPoint2D pt, unsigned short val);
void bg_pit_put_pix32 (BgPixIter *pit, IPoint2D pt, unsigned int val);

void bg_pit_scanline_fill64 (BgPixIter *pit, unsigned short len,
			     unsigned long long val);
void bg_pit_scanline_fill8 (BgPixIter *pit, unsigned short len,
			    unsigned char val);
void bg_pit_scanline_fill16 (BgPixIter *pit, unsigned short len,
			     unsigned short val);
void bg_pit_scanline_fill32 (BgPixIter *pit, unsigned short len,
			     unsigned int val);
void bg_pit_scanline_rfill64 (BgPixIter *pit, unsigned short len,
			      unsigned long long val);
void bg_pit_scanline_rfill8 (BgPixIter *pit, unsigned short len,
			     unsigned char val);
void bg_pit_scanline_rfill16 (BgPixIter *pit, unsigned short len,
			      unsigned short val);
void bg_pit_scanline_rfill32 (BgPixIter *pit, unsigned short len,
			      unsigned int val);
void bg_pit_scanline_arfill64 (BgPixIter *pit, unsigned short len,
			       unsigned long long val);
void bg_pit_scanline_arfill8 (BgPixIter *pit, unsigned short len,
			      unsigned char val);
void bg_pit_scanline_arfill16 (BgPixIter *pit, unsigned short len,
			       unsigned short val);
void bg_pit_scanline_arfill32 (BgPixIter *pit, unsigned short len,
			       unsigned int val);
void bg_pit_clear_img64 (BgPixIter *pit, unsigned long long color);
void bg_ctx8_clear_img (BgPixIterCol8 *ctx);

void bg_line_iter_y_init (BgLineIterY *lit, Point2D p1, Point2D p2);
unsigned char bg_line_iter_y_first (BgLineIterY *lit);
unsigned char bg_line_iter_y_next (BgLineIterY *lit);
void bg_pit_lineto64 (BgPixIter *pit, IPoint2D p2, unsigned long long val);
void bg_pit_tri_line64 (BgPixIter *pit,
			IPoint2D p1, IPoint2D p2, IPoint2D p3,
			unsigned long long val);
void bg_pit_quad_line64 (BgPixIter *pit,
			 IPoint2D p1, IPoint2D p2, IPoint2D p3, IPoint2D p4,
			 unsigned long long val);
void bg_pit_tri_fill64 (BgPixIter *pit,
			IPoint2D p1, IPoint2D p2, IPoint2D p3,
			unsigned long long val);

#endif /* not BOOTGRAPH_H */
