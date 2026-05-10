#ifndef LCD_MCAL_H
#define LCD_MCAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  BLK = 0U, /* BLACK */
  WHT,      /* WHITE */
  RED,      /* RED */
  GRE,      /* GREEN */
  BLE,      /* BLUE */
  YEL,      /* YELLOW */
  ORA,      /* ORANGE (saffron / kesari tint) */
  CYN,      /* CYAN */
  MAG,      /* MAGENTA */
  BRD,      /* BROWN */
  BRR,      /* BRRED */
  GRY,      /* GRAY */
  DBL,      /* DARKBLUE */
  LBL,      /* LIGHTBLUE */
  GBL,      /* GRAYBLUE */
  DGR,      /* DARKGRAY */
  LGR,      /* LIGHTGRAY */
  NAV,      /* NAVY */
  GLD,      /* GOLD */
  PRP,      /* PURPLE */
  PNK,      /* PINK */
  TEA,      /* TEAL */
  OLV,      /* OLIVE */
  MRN,      /* MAROON */
  LIM       /* LIME */
} lcd_stm32h7_color_t;

void lcd_stm32h7_init(void);
void lcd_stm32h7_backlight(uint32_t percent);
void lcd_stm32h7_breathin(void);
void lcd_stm32h7_breathout(void);
void lcd_stm32h7_color(lcd_stm32h7_color_t bg, lcd_stm32h7_color_t fg);
void lcd_stm32h7_clear(void);
uint8_t lcd_stm32h7_size(uint8_t level);
void lcd_stm32h7_char(char ch);
void lcd_stm32h7_message(const char *msg);

/** Pack 8-bit RGB into RGB565 for FillRect / SetPixel (r,g,b each 0–255). */
uint16_t lcd_mcal_rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b);

/** MCAL palette entry → RGB565 (includes ORA). */
uint16_t lcd_mcal_color565(lcd_stm32h7_color_t color);

#ifdef __cplusplus
}
#endif

#endif /* LCD_MCAL_H */
