#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include "dig7.h"

int do_quit = 0;
int do_soft_quit = 1;

unsigned int *gpio_mem;

/* From BCM2835 data-sheet, p.91 */
/* N.B. To avoid memory alignment issues, we change these to 32-bit
   integer offsets.  */
const unsigned GPFSEL_OFFSET   = 0x00 >> 2;
const unsigned GPSET_OFFSET    = 0x1c >> 2;
const unsigned GPCLR_OFFSET    = 0x28 >> 2;
const unsigned GPLEV_OFFSET    = 0x34 >> 2;
const unsigned GPEDS_OFFSET    = 0x40 >> 2;
const unsigned GPREN_OFFSET    = 0x4c >> 2;
const unsigned GPFEN_OFFSET    = 0x58 >> 2;
const unsigned GPHEN_OFFSET    = 0x64 >> 2;
const unsigned GPLEN_OFFSET    = 0x70 >> 2;
const unsigned GPAREN_OFFSET   = 0x7c >> 2;
const unsigned GPAFEN_OFFSET   = 0x88 >> 2;
const unsigned GPPUD_OFFSET    = 0x94 >> 2;
const unsigned GPPUDCLK_OFFSET = 0x98 >> 2;
const unsigned char N = 4;

enum {
  GPFN_INPUT,
  GPFN_OUTPUT,
  GPFN_ALT5,
  GPFN_ALT4,
  GPFN_ALT0,
  GPFN_ALT1,
  GPFN_ALT2,
  GPFN_ALT3,
};

enum {
  GPUL_OFF,
  GPUL_DOWN,
  GPUL_UP,
};

/* 3x5 pixel font designed to be mostly readable on 7-segment LCDs by
   a simple means of selecting a subset of available pixels.  Index
   zero = ASCII 0x20.  */
/* Corresponds to a sans-serif font on 7-segment LCDs.  */
const PixChar3x5 g_pf3x5_seg7[102] = {
  /* SPACE */
  { 0,0,0,
    0,0,0,
    0,0,0,
    0,0,0,
    0,0,0, },
  /* ! */
  { 0,1,0,
    0,1,0,
    0,1,0,
    0,0,0,
    0,1,0, },
  /* " */
  { 1,0,1,
    1,0,1,
    0,0,0,
    0,0,0,
    0,0,0, },
  /* # 3x5 HARD TO READ */
  { 1,0,1,
    1,1,1,
    1,0,1,
    1,1,1,
    1,0,1, },
  /* $ 3x5 HARD TO READ */
  { 1,1,1,
    1,1,0,
    1,1,1,
    0,1,1,
    1,1,1, },
  /* % */
  { 1,0,1,
    0,0,1,
    0,1,0,
    1,0,0,
    1,0,1, },
  /* & */
  { 0,1,1,
    0,1,0,
    1,1,0,
    1,0,1,
    1,1,1, },
  /* ' */
  { 0,1,0,
    0,1,0,
    0,0,0,
    0,0,0,
    0,0,0, },
  /* ( */
  { 0,0,1,
    0,1,0,
    0,1,0,
    0,1,0,
    0,0,1, },
  /* ) */
  { 1,0,0,
    0,1,0,
    0,1,0,
    0,1,0,
    1,0,0, },
  /* * 3x5 HARD TO READ */
  { 0,1,0,
    1,1,1,
    1,1,1,
    1,1,1,
    0,1,0, },
  /* + */
  { 0,0,0,
    0,1,0,
    1,1,1,
    0,1,0,
    0,0,0, },
  /* , */
  { 0,0,0,
    0,0,0,
    0,1,0,
    0,1,0,
    1,0,0, },
  /* - */
  { 0,0,0,
    0,0,0,
    1,1,1,
    0,0,0,
    0,0,0, },
  /* . */
  { 0,0,0,
    0,0,0,
    0,0,0,
    0,1,0,
    0,0,0, },
  /* / */
  { 0,0,1,
    0,0,1,
    0,1,0,
    1,0,0,
    1,0,0, },

  /* 0 */
  { 1,1,1,
    1,0,1,
    1,0,1,
    1,0,1,
    1,1,1, },
  /* 1 */
  { 0,0,1,
    0,1,1,
    0,0,1,
    0,0,1,
    0,0,1, },
  /* 2 */
  { 0,1,1,
    0,0,1,
    0,1,1,
    1,0,0,
    1,1,1, },
  /* 3 */
  { 1,1,0,
    0,0,1,
    0,1,0,
    0,0,1,
    1,1,0, },
  /* 4 */
  { 1,0,1,
    1,0,1,
    1,1,1,
    0,0,1,
    0,0,1, },
  /* 5 */
  { 1,1,1,
    1,0,0,
    1,1,1,
    0,0,1,
    1,1,1, },
  /* 6 */
  { 1,1,1,
    1,0,0,
    1,1,1,
    1,0,1,
    1,1,1, },
  /* 7 */
  { 1,1,1,
    0,0,1,
    0,1,0,
    1,0,0,
    1,0,0, },
  /* 8 */
  { 1,1,1,
    1,0,1,
    1,1,1,
    1,0,1,
    1,1,1, },
  /* 9 */
  { 1,1,1,
    1,0,1,
    1,1,1,
    0,0,1,
    0,0,1, },

  /* : */
  { 0,0,0,
    0,1,0,
    0,0,0,
    0,1,0,
    0,0,0, },
  /* ; 3x5 HOMOGLYPH */
  { 0,1,0,
    0,0,0,
    0,1,0,
    0,1,0,
    1,0,0, },
  /* < */
  { 0,0,1,
    0,1,0,
    1,0,0,
    0,1,0,
    0,0,1, },
  /* = */
  { 0,0,0,
    1,1,1,
    0,0,0,
    1,1,1,
    0,0,0, },
  /* > */
  { 1,0,0,
    0,1,0,
    0,0,1,
    0,1,0,
    1,0,0, },
  /* ? */
  { 0,1,0,
    1,0,1,
    0,0,1,
    0,1,0,
    0,1,0, },
  /* @ 3x5 HARD TO READ */
  { 1,1,1,
    1,0,1,
    1,0,1,
    1,0,0,
    1,1,1, },

  /* A */
  { 1,1,1,
    1,0,1,
    1,1,1,
    1,0,1,
    1,0,1, },
  /* B */
  { 1,1,0,
    1,0,1,
    1,1,0,
    1,0,1,
    1,1,0, },
  /* C */
  { 0,1,1,
    1,0,0,
    1,0,0,
    1,0,0,
    0,1,1, },
  /* D */
  { 1,1,0,
    1,0,1,
    1,0,1,
    1,0,1,
    1,1,0, },
  /* E */
  { 1,1,1,
    1,0,0,
    1,1,0,
    1,0,0,
    1,1,1, },
  /* F */
  { 1,1,1,
    1,0,0,
    1,1,0,
    1,0,0,
    1,0,0, },
  /* G */
  { 0,1,1,
    1,0,0,
    1,0,1,
    1,0,1,
    0,1,1, },
  /* H */
  { 1,0,1,
    1,0,1,
    1,1,1,
    1,0,1,
    1,0,1, },
  /* I 7-SEG UNREADABLE */
  { 1,1,1,
    0,1,0,
    0,1,0,
    0,1,0,
    1,1,1, },
  /* J */
  { 0,0,1,
    0,0,1,
    0,0,1,
    1,0,1,
    1,1,1, },
  /* K */
  { 1,0,1,
    1,0,1,
    1,1,0,
    1,0,1,
    1,0,1, },
  /* L */
  { 1,0,0,
    1,0,0,
    1,0,0,
    1,0,0,
    1,1,1, },
  /* M */
  { 1,0,1,
    1,1,1,
    1,1,1,
    1,0,1,
    1,0,1, },
  /* N */
  { 1,0,0,
    1,0,1,
    1,1,1,
    1,0,1,
    0,0,1, },
  /* O */
  { 0,1,0,
    1,0,1,
    1,0,1,
    1,0,1,
    0,1,0, },
  /* P */
  { 1,1,1,
    1,0,1,
    1,1,1,
    1,0,0,
    1,0,0, },
  /* Q */
  { 0,1,0,
    1,0,1,
    1,0,1,
    1,1,1,
    0,0,1, },
  /* R */
  { 1,1,0,
    1,0,1,
    1,1,0,
    1,0,1,
    1,0,1, },
  /* S */
  { 1,1,0,
    1,0,0,
    1,1,1,
    0,0,1,
    0,1,1, },
  /* T 7-SEG UNREADABLE */
  { 1,1,1,
    0,1,0,
    0,1,0,
    0,1,0,
    0,1,0, },
  /* U */
  { 1,0,1,
    1,0,1,
    1,0,1,
    1,0,1,
    1,1,1, },
  /* V 7-SEG UNREADABLE */
  { 1,0,1,
    1,0,1,
    1,0,1,
    0,1,0,
    0,1,0, },
  /* W */
  { 1,0,1,
    1,0,1,
    1,1,1,
    1,1,1,
    1,0,1, },
  /* X 7-SEG UNREADABLE */
  { 1,0,1,
    1,0,1,
    0,1,0,
    1,0,1,
    1,0,1, },
  /* Y 7-SEG UNREADABLE */
  { 1,0,1,
    1,0,1,
    0,1,0,
    0,1,0,
    0,1,0, },
  /* Z 7-SEG HOMOGLYPH */
  { 1,1,1,
    0,0,1,
    0,1,0,
    1,0,0,
    1,1,1, },

  /* [ */
  { 1,1,0,
    1,0,0,
    1,0,0,
    1,0,0,
    1,1,0, },
  /* \ */
  { 1,0,0,
    1,0,0,
    0,1,0,
    0,0,1,
    0,0,1, },
  /* ] */
  { 0,1,1,
    0,0,1,
    0,0,1,
    0,0,1,
    0,1,1, },
  /* ^ */
  { 0,1,0,
    1,0,1,
    0,0,0,
    0,0,0,
    0,0,0, },
  /* _ */
  { 0,0,0,
    0,0,0,
    0,0,0,
    0,0,0,
    1,1,1, },
  /* ` */
  { 1,0,0,
    0,1,0,
    0,0,0,
    0,0,0,
    0,0,0, },

  /* a */
  { 0,0,0,
    0,0,0,
    0,1,1,
    1,0,1,
    0,1,1, },
  /* b */
  { 1,0,0,
    1,0,0,
    1,1,0,
    1,0,1,
    1,1,0, },
  /* c */
  { 0,0,0,
    0,0,0,
    0,1,1,
    1,0,0,
    0,1,1, },
  /* d */
  { 0,0,1,
    0,0,1,
    0,1,1,
    1,0,1,
    0,1,1, },
  /* e */
  { 0,1,0,
    1,0,1,
    1,1,1,
    1,0,0,
    0,1,1, },
  /* f */
  { 0,1,0,
    1,0,1,
    1,0,0,
    1,1,0,
    1,0,0, },
  /* g 7-SEG EASY TO CONFUSE */
  { 0,1,1,
    1,0,1,
    0,1,1,
    0,0,1,
    0,1,0, },
  /* h */
  { 1,0,0,
    1,0,0,
    1,1,0,
    1,0,1,
    1,0,1, },
  /* i */
  { 0,0,0,
    0,1,0,
    0,0,0,
    0,1,0,
    0,1,0, },
  /* j */
  { 0,0,1,
    0,0,0,
    0,0,1,
    1,0,1,
    0,1,0, },
  /* k */
  { 1,0,0,
    1,0,0,
    1,0,1,
    1,1,0,
    1,0,1, },
  /* l */
  { 0,1,0,
    0,1,0,
    0,1,0,
    0,1,0,
    0,1,0, },
  /* m */
  { 0,0,0,
    0,0,0,
    1,0,0,
    1,1,1,
    1,1,1, },
  /* n */
  { 0,0,0,
    0,0,0,
    1,1,0,
    1,0,1,
    1,0,1, },
  /* o */
  { 0,0,0,
    0,0,0,
    0,1,0,
    1,0,1,
    0,1,0, },
  /* p */
  { 0,0,0,
    1,1,0,
    1,0,1,
    1,1,0,
    1,0,0, },
  /* q 7-SEG HOMOGLYPH */
  { 0,1,1,
    1,0,1,
    0,1,1,
    0,0,1,
    0,0,1, },
  /* r */
  { 0,0,0,
    0,0,0,
    1,1,0,
    1,0,0,
    1,0,0, },
  /* s */
  { 0,0,0,
    0,1,0,
    1,0,0,
    0,1,0,
    1,0,0, },
  /* t */
  { 0,1,0,
    0,1,0,
    1,1,1,
    0,1,0,
    0,1,0, },
  /* u */
  { 0,0,0,
    0,0,0,
    1,0,1,
    1,0,1,
    0,1,1, },
  /* v */
  { 0,0,0,
    0,0,0,
    1,0,1,
    1,0,1,
    0,1,0, },
  /* w */
  { 0,0,0,
    0,0,0,
    1,1,1,
    1,1,1,
    0,1,0, },
  /* x */
  { 0,0,0,
    0,0,0,
    1,0,1,
    0,1,0,
    1,0,1, },
  /* y */
  { 1,0,1,
    1,0,1,
    0,1,1,
    0,0,1,
    0,1,0, },
  /* z */
  { 0,0,0,
    1,1,0,
    0,1,0,
    1,0,0,
    1,1,0, },

  /* { */
  { 0,1,1,
    0,1,0,
    1,0,0,
    0,1,0,
    0,1,1, },
  /* | */
  { 0,1,0,
    0,1,0,
    0,0,0,
    0,1,0,
    0,1,0, },
  /* } */
  { 1,1,0,
    0,1,0,
    0,0,1,
    0,1,0,
    1,1,0, },
  /* ~ 3x5 HARD TO READ */
  { 0,0,0,
    1,0,0,
    1,1,1,
    0,0,1,
    0,0,0, },

  /* DEL, but used as "undefined character" here */
  { 1,1,1,
    1,1,1,
    1,1,1,
    1,1,1,
    1,1,1, },

  /* 7, 3x5 COMPROMISE for 7-segment compatibility */
  { 1,1,1,
    0,0,1,
    0,0,1,
    0,0,1,
    0,0,1, },

  /* t, 3x5 COMPROMISE for 7-segment compatibility */
  { 0,0,1,
    0,0,1,
    0,1,1,
    0,0,1,
    0,0,1, },

  /* "Intersection" or M 3x5 COMPROMISE for 7-segment compatibility */
  { 0,1,0,
    1,0,1,
    1,0,1,
    1,0,1,
    1,0,1, },

  /* "Union" or W 3x5 COMPROMISE for 7-segment compatibility */
  { 1,0,1,
    1,0,1,
    1,0,1,
    1,0,1,
    0,1,0, },

  /* k 3x5 COMPROMISE for 7-segment compatibility */
  /* Alas, it looks like a lowercase t in a serif font */
  { 1,0,0,
    1,0,0,
    1,1,0,
    1,0,0,
    1,1,0, },

  /* k 3x5 COMPROMISE for 7-segment compatibility */
  /* Alas, it looks like a lowercase t in a serif font */
  { 1,0,0,
    1,0,0,
    1,0,1,
    1,0,1,
    1,0,1, },

  /* TODO: v alt-glyph */

};

void
seg7_draw_text (char *buffer, const PixChar3x5 *font,
                unsigned x, unsigned y,
                const char *text, unsigned len)
{
  unsigned i;
  for (i = 0; i < len; i++) {
    unsigned char ch = text[i];
    if (ch < 0x20)
      ch = 0x20;
    ch -= 0x20;
    gpio_set_digit (i);
    gpio_clear_segs ();
    gpio_set_seg (0, font[ch].p01);
    gpio_set_seg (1, font[ch].p10);
    gpio_set_seg (2, font[ch].p12);
    gpio_set_seg (3, font[ch].p21);
    gpio_set_seg (4, font[ch].p30);
    gpio_set_seg (5, font[ch].p32);
    gpio_set_seg (6, font[ch].p41);
    gpio_flush_digit ();
  }
}

/* Draw 180 degree rotated to a GPIO buffer.  */
void
seg7_draw_text_rot180 (char *buffer, const PixChar3x5 *font,
                       unsigned x, unsigned y,
                       const char *text, unsigned len)
{
  unsigned i;
  for (i = 0; i < len; i++) {
    unsigned char ch = text[i];
    if (ch < 0x20)
      ch = 0x20;
    ch -= 0x20;
    gpio_set_digit (i);
    gpio_clear_segs ();
    gpio_set_seg (6, font[ch].p01);
    gpio_set_seg (5, font[ch].p10);
    gpio_set_seg (4, font[ch].p12);
    gpio_set_seg (3, font[ch].p21);
    gpio_set_seg (2, font[ch].p30);
    gpio_set_seg (1, font[ch].p32);
    gpio_set_seg (0, font[ch].p41);
    gpio_flush_digit ();
  }
}

int
rpi_gpio_init (void)
{
  int result;
  int fd = open ("/dev/gpiomem", O_RDWR | O_SYNC);
  if (fd == -1)
    return 0;
  gpio_mem = mmap (NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (gpio_mem == (unsigned int*)-1)
    return 0;
  return 1;
}

void
rpi_gpio_set_fn (unsigned char idx, unsigned char fn)
{
  unsigned word_idx = idx / 10;
  unsigned int wordbuf = gpio_mem[GPFSEL_OFFSET+word_idx];
  wordbuf &= ~(0x07 << ((idx % 10) * 3));
  wordbuf |= (fn & 0x07) << ((idx % 10) * 3);
  gpio_mem[GPFSEL_OFFSET+word_idx] = wordbuf;
}

void
rpi_gpio_set_pull (unsigned char idx, unsigned char pull)
{
  unsigned int wordbuf;
  unsigned i;
  gpio_mem[GPPUD_OFFSET] = (unsigned int)pull & 0x03;
  /* Wait at least 150 cycles.  */
  for (i = 150; i > 0; i--);
  wordbuf = 1 << idx;
  gpio_mem[GPPUDCLK_OFFSET] = wordbuf;
  /* Wait at least 150 cycles.  */
  for (i = 150; i > 0; i--);
  gpio_mem[GPPUD_OFFSET] = (unsigned int)GPUL_OFF;
  gpio_mem[GPPUDCLK_OFFSET] = 0;
}

void
rpi_gpio_set_pin (unsigned char idx, unsigned char val)
{
  /* N.B. Do not read the current value and use that to set the new
     value else you get problems with random junk.  Only set/clear the
     value you want to change.  */
  if (val) { /* set the pin to 1 */
    unsigned int wordbuf = gpio_mem[GPSET_OFFSET];
    /* wordbuf |= 1 << idx; */
    wordbuf = 1 << idx;
    gpio_mem[GPSET_OFFSET] = wordbuf;
  } else { /* clear the pin to zero */
    unsigned int wordbuf = gpio_mem[GPCLR_OFFSET];
    /* wordbuf |= 1 << idx; */
    wordbuf = 1 << idx;
    gpio_mem[GPCLR_OFFSET] = wordbuf;
  }
}

unsigned char
rpi_gpio_get_pin (unsigned char idx)
{
  unsigned int wordbuf = gpio_mem[GPLEV_OFFSET];
  /* N.B. Interpret the values as follows.  The value of the pin is
     the current flowing through the pull-up/down termination.  For
     example:

     * If you have pull-up termination, the value is one when the
       switch is open, zero when the switch is closed.

     * If you have pull-down termination, the value is zero when the
       switch is open, one when the switch is closed.  */
  return (wordbuf >> idx) & 1;
}

unsigned char
rpi_gpio_get_pin_event (unsigned char idx)
{
  unsigned int wordbuf = gpio_mem[GPEDS_OFFSET];
  return (wordbuf >> idx) & 1;
}

void
rpi_gpio_clear_pin_event (unsigned char idx)
{
  gpio_mem[GPEDS_OFFSET] = 1 << idx;
}

/* Watch for rising edge.  */
void
rpi_gpio_watch_re (unsigned char idx)
{
  gpio_mem[GPREN_OFFSET] |= 1 << idx;
}

void
rpi_gpio_unwatch_re (unsigned char idx)
{
  gpio_mem[GPREN_OFFSET] &= ~(1 << idx);
}

/* Watch for falling edge.  */
void
rpi_gpio_watch_fe (unsigned char idx)
{
  gpio_mem[GPFEN_OFFSET] |= 1 << idx;
}

void
rpi_gpio_unwatch_fe (unsigned char idx)
{
  gpio_mem[GPFEN_OFFSET] &= ~(1 << idx);
}

void
gpio_clear_segs (void)
{
  unsigned char segs[8] = { 10, 9, 24, 11, 23, 27, 22, 18 };
  unsigned i;
  for (i = 0; i < 8; i++) {
    rpi_gpio_set_pin (segs[i], 0);
  }
}

void
gpio_init_segs (void)
{
  unsigned char segs[8] = { 10, 9, 24, 11, 23, 27, 22, 18 };
  unsigned i;
  for (i = 0; i < 8; i++) {
    rpi_gpio_set_fn (segs[i], GPFN_OUTPUT);
    rpi_gpio_set_pull (segs[i], GPUL_UP);
    rpi_gpio_set_pin (segs[i], 0);
  }
}

void
gpio_close_segs (void)
{
  unsigned char segs[8] = { 10, 9, 24, 11, 23, 27, 22, 18 };
  unsigned i;
  for (i = 0; i < 8; i++) {
    rpi_gpio_set_pin (segs[i], 0);
    rpi_gpio_set_pull (segs[i], GPUL_OFF);
    /* rpi_gpio_set_fn (segs[i], GPFN_INPUT); */
  }
}

/*
   g f G a b
   | | | | |
      aaa         000
     f   b       1   2
      ggg         333
     e   c       4   5
      ddd  h      666
   | | | | |
   e d G c h

GPIO pin assignments:

G = GND
a = 10
b = 24
c = 27
d = 22
e = 23
f = 9
g = 11
h = 18

*/
void
gpio_set_seg (unsigned char idx, unsigned char val)
{
  switch (idx) {
  case 0: rpi_gpio_set_pin (10, val); break;
  case 1: rpi_gpio_set_pin (9, val); break;
  case 2: rpi_gpio_set_pin (24, val); break;
  case 3: rpi_gpio_set_pin (11, val); break;
  case 4: rpi_gpio_set_pin (23, val); break;
  case 5: rpi_gpio_set_pin (27, val); break;
  case 6: rpi_gpio_set_pin (22, val); break;
  default: break;
  }
}

void
gpio_set_digit (unsigned char idx)
{
  return;
  rpi_gpio_set_pin (N, 0);
  rpi_gpio_set_pin (N, 0);
  rpi_gpio_set_pin (N, 0);
  rpi_gpio_set_pin (N, 0);
  switch (idx) {
  case 0: rpi_gpio_set_pin (N, 1); break;
  case 1: rpi_gpio_set_pin (N, 1); break;
  case 2: rpi_gpio_set_pin (N, 1); break;
  case 3: rpi_gpio_set_pin (N, 1); break;
  default: break;
  }
}

void
gpio_flush_digit (void)
{
  /* If using a shift register, shift out the digit values now.
     Otherwise, do nothing.  */
}

void
handle_sigint (int signum)
{
  do_quit = 1;
}

void
slow_key_scan (int signum)
{
  const unsigned char POPIN = 25;
  if (rpi_gpio_get_pin_event (POPIN)) {
    rpi_gpio_clear_pin_event (POPIN);
    do_soft_quit = !do_soft_quit;
  }
  alarm (1);
}

int
main (int argc, char *argv[])
{
  const unsigned char MYPIN = 4;
  const unsigned char YOPIN = 17;
  const unsigned char POPIN = 25;
  const unsigned char PWPIN = 8;
  unsigned char mode = 0;
  int speed = 0;
  signed char speed_step = 1;
  unsigned i;

  /* Mode flags:
     1: Rotate display 180 degrees
     2: Endless loop
     4: Speed up gradually, slow down gradually, loop
     8: Do not use input signal
     16: No sound
  */
  if (argc >= 2) {
    mode = (unsigned char)atoi (argv[1]);
  }
  if (argc >= 3) {
    speed = atoi (argv[2]);
  }
  if (!rpi_gpio_init ())
    return 1;

  {
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction (SIGINT, &sa, NULL) ==  -1)
      return 1;

    sa.sa_handler = slow_key_scan;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction (SIGALRM, &sa, NULL) ==  -1)
      return 1;

    alarm (1);
  }

  rpi_gpio_set_fn (POPIN, GPFN_INPUT);
  rpi_gpio_set_pull (POPIN, GPUL_UP);
  rpi_gpio_watch_fe (POPIN);
  rpi_gpio_clear_pin_event (POPIN);

  while (!do_quit) {
    if ((mode & 8) != 8 && do_soft_quit) {
      pause ();
      continue;
    }

    rpi_gpio_set_fn (MYPIN, GPFN_OUTPUT);
    rpi_gpio_set_pull (MYPIN, GPUL_UP);
    rpi_gpio_set_fn (YOPIN, GPFN_OUTPUT);
    rpi_gpio_set_pull (YOPIN, GPUL_UP);
    if ((mode & 16) != 16) {
      rpi_gpio_set_fn (PWPIN, GPFN_OUTPUT);
      rpi_gpio_set_pull (PWPIN, GPUL_UP);
    }
    gpio_init_segs ();

    for (i = 0; !do_quit &&
           ((mode & 8) == 8 || !do_soft_quit) &&
           i < 36; i++) {
      struct timespec sleep_time = { 1, 0 };
      char ch;
      if (i < 10) {
        if (i == 7)
          ch = 0x80; /* use alternate '7' font glyph */
        else
          ch = '0' + i;
      }
      else {
        /* Use lower-case for select letters, and '1' for 'I'.  */
        /* Unreadable characters that appear like 'H': K M W X */
        /* Unreadable characters, horizontal lines: I T Y */
        /* Unreadable characters, unique: k V */
        /* 'V' is curiously somewhat readable.  */
        /* N.B. 7 is displayed a bit weird with this font.  */
        switch (i) {
        case 11: ch = 'b'; break; /* else looks like '8' */
        case 13: ch = 'd'; break; /* else looks like '0' */
          /* case 16: ch = 'g'; break; */ /* else hard to read */
        case 17: ch = 'h'; break; /* else too easy to confuse */
        case 18: ch = '1'; break; /* else horizontal lines */
        case 20: ch = 0x85; break; /* else looks like 'H' */
        case 22: ch = 0x82; break;
        case 23: ch = 'n'; break; /* else looks like 'H' */
        case 24: ch = 'o'; break; /* else looks like '0' */
        case 26: ch = 'q'; break; /* else looks like 'M' alt-glyph */
        case 27: ch = 'r'; break; /* else looks like 'A' */
        case 29: ch = 0x81; break; /* else horizontal lines */
          /* Serif t, looks prettier but seems to be less readable */
          /* case 29: ch = 0x83; break; */ /* else horizontal lines */
        case 30: ch = 'u'; break; /* else too easy to confuse */
        case 32: ch = 0x83; break;
        case 34: ch = 'y'; break; /* else horizontal lines */
        default: ch = 'A' + i - 10; break;
        }
      }
      if ((mode & 1) != 1)
        seg7_draw_text (NULL, g_pf3x5_seg7, 0, 0, &ch, 1);
      else /* (mode & 1) == 1 */
        seg7_draw_text_rot180 (NULL, g_pf3x5_seg7, 0, 0, &ch, 1);
      rpi_gpio_set_pin (MYPIN, i & 1);
      rpi_gpio_set_pin (YOPIN, i & 2);
      if ((mode & 16) != 16)
        rpi_gpio_set_pin (PWPIN, i & 1);
      if (speed >= 2) {
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = 1000000000 / speed;
      } else if (speed <= -2) {
        sleep_time.tv_sec *= -speed;
      }
      clock_nanosleep (CLOCK_MONOTONIC, 0 /* TIMER_RELTIME */,
                       &sleep_time, NULL);
      if ((mode & 2) == 2 && i == 35) i = (unsigned)-1;
      if ((mode & 4) == 4) {
        /* Timing 100 microseconds is accurate, 10 microseconds is
           somewhat accurate, and 1 microsecond is very inaccurate.
           Unless you use more advanced realtime techniques.  */
        if (speed >= 10000) {
          speed_step = -1;
        }
        if (speed <= 0) {
          speed_step = 1;
        }
        speed += speed_step;
      }
    }

    rpi_gpio_set_pin (MYPIN, 0);
    rpi_gpio_set_pull (MYPIN, GPUL_OFF);
    /* rpi_gpio_set_fn (MYPIN, GPFN_INPUT); */
    rpi_gpio_set_pin (YOPIN, 0);
    rpi_gpio_set_pull (YOPIN, GPUL_OFF);
    /* rpi_gpio_set_fn (YOPIN, GPFN_INPUT); */
    if ((mode & 16) != 16) {
      rpi_gpio_set_pin (PWPIN, 0);
      rpi_gpio_set_pull (PWPIN, GPUL_OFF);
      /* rpi_gpio_set_fn (PWPIN, GPFN_INPUT); */
    }

    if ((mode & 8) == 8)
      do_quit = 1;

    if (!do_quit  && do_soft_quit) {
      char *shdn_str = " Shdn321";
      while (!do_quit && do_soft_quit && *shdn_str) {
        if ((mode & 1) != 1)
          seg7_draw_text (NULL, g_pf3x5_seg7, 0, 0, shdn_str++, 1);
        else /* (mode & 1) == 1 */
          seg7_draw_text_rot180 (NULL, g_pf3x5_seg7, 0, 0, shdn_str++, 1);
        sleep (1);
      }
      /* Verify that the user has not cancelled the process in the
         countdown.  */
      if (!do_quit && do_soft_quit) {
        gpio_close_segs ();

        rpi_gpio_clear_pin_event (POPIN);
        rpi_gpio_unwatch_fe (POPIN);
        rpi_gpio_set_pin (POPIN, 0);
        rpi_gpio_set_pull (POPIN, GPUL_OFF);
        /* rpi_gpio_set_fn (POPIN, GPFN_INPUT); */

        system ("sudo shutdown -h now");
        return 0;
      }
    }
    do_soft_quit = 1;

    gpio_close_segs ();

  }

  rpi_gpio_clear_pin_event (POPIN);
  rpi_gpio_unwatch_fe (POPIN);
  rpi_gpio_set_pin (POPIN, 0);
  rpi_gpio_set_pull (POPIN, GPUL_OFF);
  /* rpi_gpio_set_fn (POPIN, GPFN_INPUT); */

  return 0;
}

/* CLEVER TRICK: Use AND-XOR masking to draw GPIO masks corresponding
   to 7-segment fonts.  */

/* Marquee?  Nah, we know that's been shunned in modern graphics and
   web programming on the big systems.  So, what kind of alternative
   can we use when our display area is literally too limited to
   display an important long full message?

   Follow in the footsteps of the web programming and JavaScript
   approach.  First of all, define a scrollable text area.  This is a
   widget that can be rendered.  Then, define API methods on it.
   "Marquee" is then implemented as a "non-standard" subroutine on top
   of these base types.  And rightfully so because it only applies
   when you have both limited display area and limited direct
   interaction from the user at the same time, which is far from
   common in modern computing.  */
