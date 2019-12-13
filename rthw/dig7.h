#ifndef DIG7_H
#define DIG7_H

struct PixChar3x5_tag
{
  /* pMN: M = row, N = column */
  unsigned short p00 : 1;
  unsigned short p01 : 1;
  unsigned short p02 : 1;

  unsigned short p10 : 1;
  unsigned short p11 : 1;
  unsigned short p12 : 1;

  unsigned short p20 : 1;
  unsigned short p21 : 1;
  unsigned short p22 : 1;

  unsigned short p30 : 1;
  unsigned short p31 : 1;
  unsigned short p32 : 1;

  unsigned short p40 : 1;
  unsigned short p41 : 1;
  unsigned short p42 : 1;
};
typedef struct PixChar3x5_tag PixChar3x5;

extern const PixChar3x5 g_pf3x5_seg7[102];

void
seg7_draw_text (char *buffer, const PixChar3x5 *font,
                unsigned x, unsigned y,
                const char *text, unsigned len);
void
seg7_draw_text_rot180 (char *buffer, const PixChar3x5 *font,
                       unsigned x, unsigned y,
                       const char *text, unsigned len);
int
rpi_gpio_init (void);
void
rpi_gpio_set_fn (unsigned char idx, unsigned char fn);
void
rpi_gpio_set_pull (unsigned char idx, unsigned char pull);
void
rpi_gpio_set_pin (unsigned char idx, unsigned char val);
unsigned char
rpi_gpio_get_pin (unsigned char idx);
unsigned char
rpi_gpio_get_pin_event (unsigned char idx);
void
rpi_gpio_clear_pin_event (unsigned char idx);
void
rpi_gpio_watch_re (unsigned char idx);
void
rpi_gpio_unwatch_re (unsigned char idx);
void
rpi_gpio_watch_fe (unsigned char idx);
void
rpi_gpio_unwatch_fe (unsigned char idx);
void
gpio_clear_segs (void);
void
gpio_init_segs (void);
void
gpio_close_segs (void);
void
gpio_set_seg (unsigned char idx, unsigned char val);
void
gpio_set_digit (unsigned char idx);
void
gpio_flush_digit (void);

#endif /* not DIG7_H */
