#ifndef __LCD_H
#define __LCD_H

#include "main.h"
#include "st7735.h"
#include <stdio.h>

#if !defined(TFT96) && !defined(TFT18)
#define TFT96
#endif

#define WHITE          0xFFFF
#define BLACK          0x0000
#define BLUE           0x001F
#define BRED           0XF81F
#define GRED           0XFFE0
#define GBLUE          0X07FF
#define RED            0xF800
#define MAGENTA        0xF81F
#define GREEN          0x07E0
#define CYAN           0x7FFF
#define YELLOW         0xFFE0
#define BROWN          0XBC40
#define BRRED          0XFC07
#define GRAY           0X8430
#define DARKBLUE       0X01CF
#define LIGHTBLUE      0X7D7C
#define GRAYBLUE       0X5458
#define DARKGRAY       0x4208
#define LIGHTGRAY      0xC618
#define NAVY           0x012C
#define GOLD           0xFEA0
#define PURPLE         0x8010
#define PINK           0xFB56
#define TEAL           0x0410
#define OLIVE          0x8400
#define MAROON         0x8000
#define LIME           0x3666

#define WHT WHITE
#define BLK BLACK
#define BLE BLUE
#define BRD BRED
#define GRD GRED
#define GBL GBLUE
#define MAG MAGENTA
#define GRE GREEN
#define CYN CYAN
#define YEL YELLOW
#define BRN BROWN
#define BRR BRRED
#define GRA GRAY
#define DBL DARKBLUE
#define LBL LIGHTBLUE
#define GBY GRAYBLUE
#define DGY DARKGRAY
#define LGY LIGHTGRAY
#define NVY NAVY

extern ST7735_Object_t st7735_pObj;
extern ST7735_IO_t st7735_pIO;
extern uint32_t st7735_id;

extern uint16_t POINT_COLOR;
extern uint16_t BACK_COLOR;

void LCD_Test(void);
void LCD_SetBrightness(uint32_t Brightness);
uint32_t LCD_GetBrightness(void);
void LCD_Light(uint32_t Brightness_Dis, uint32_t time);
void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint8_t mode);
void LCD_ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, uint8_t *p);
extern ST7735_Ctx_t ST7735Ctx;

#endif
