/**
  ******************************************************************************
  * @file    lcd_mcal.h
  * @author  Sibun
  * @brief   ST7735 TFT LCD MCAL (SPI4 + TIM1 backlight) for STM32H7.
  *
  * @copyright
  * Copyright (c) 2026 Sibun. All rights reserved.
  *
  * CubeMX: SPI4, TIM1 CH2N backlight, User Labels LCD_CS / LCD_WR_RS; panel via TFT96/TFT18.
  ******************************************************************************
  */

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

/**
  * @brief  Initialize ST7735 on SPI4, reset cursor/colors, and turn backlight on via TIM1.
  * @param  None
  * @retval None
  */
void lcd_stm32h7_init(void);

/**
  * @brief  Set backlight brightness using TIM1 compare (linear map 0–100 % to ARR).
  * @param  percent Brightness 0..100; larger values are clamped to 100.
  * @retval None
  */
void lcd_stm32h7_backlight(uint32_t percent);

/**
  * @brief  Ramp backlight from 0 % to 100 % in steps (demo / idle animation).
  * @param  None
  * @retval None
  */
void lcd_stm32h7_breathin(void);

/**
  * @brief  Ramp backlight down from current level toward 0 % in steps.
  * @param  None
  * @retval None
  */
void lcd_stm32h7_breathout(void);

/**
  * @brief  Apply background and foreground colors from the MCAL palette (RGB565 internally)
  *         and immediately repaint the full LCD with the new background; text cursor
  *         is reset to (0,0).
  * @param  bg Background palette index. Used to fill the whole screen right away and as
  *            the background color for subsequent text.
  * @param  fg Foreground palette index. Used as the glyph color for subsequent text.
  * @retval None
  */
void lcd_stm32h7_color(lcd_stm32h7_color_t bg, lcd_stm32h7_color_t fg);

/**
  * @brief  Fill the screen with the current background color and reset text cursor to (0,0).
  * @param  None
  * @retval None
  */
void lcd_stm32h7_clear(void);

/**
  * @brief  Select small (12 px) or large (16 px) font; optional level 1..3 stores preference.
  * @param  level 1 = small font, 2 or 3 = large font; other values leave mode unchanged.
  * @retval Current font pixel height in Y (12 or 16) after the call.
  */
uint8_t lcd_stm32h7_size(uint8_t level);

/**
  * @brief  Draw one character at the cursor and advance cursor (wraps at line/screen edge).
  * @param  ch Character to print; '\\n' / '\\r' trigger newline handling.
  * @retval None
  */
void lcd_stm32h7_char(char ch);

/**
  * @brief  Print a null-terminated string using the current font and colors.
  * @param  msg Pointer to C string; NULL is ignored safely.
  * @retval None
  */
void lcd_stm32h7_message(const char *msg);

/**
  * @brief  Redraw a changing tail at the current cursor column: first call snaps the column/row;
  *         later calls clear to the right edge and redraw (e.g. after "QSPI Flash :- " print %).
  * @param  msg Null-terminated string; NULL is ignored. Re-arms from cursor if the text row changed.
  * @retval None
  */
void lcd_stm32h7_dynamic_update(const char *msg);

/**
  * @brief  Convert 8-bit RGB channels to RGB565 for low-level fill / pixel helpers.
  * @param  r Red 0–255.
  * @param  g Green 0–255.
  * @param  b Blue 0–255.
  * @retval RGB565 pixel value.
  */
uint16_t lcd_mcal_rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b);

/**
  * @brief  Map MCAL palette enum (BLK, WHT, ORA, …) to RGB565 including custom ORA tint.
  * @param  color Palette entry @ref lcd_stm32h7_color_t.
  * @retval RGB565 color for ST7735 drawing.
  */
uint16_t lcd_mcal_color565(lcd_stm32h7_color_t color);

#ifdef __cplusplus
}
#endif

#endif /* LCD_MCAL_H */
