/**
  ******************************************************************************
  * @file    main_lcd_message.c
  * @author  Sibun
  * @brief   Bootloader LCD messages (main path).
  *
  * @copyright
  * Copyright (c) 2026 Sibun. All rights reserved.
  ******************************************************************************
  */

#include "main_lcd_message.h"
#include "lcd_mcal.h"
#include "main.h"

void main_bootloader_msg(void)
{
  lcd_stm32h7_init();
  lcd_stm32h7_backlight(100);
  lcd_stm32h7_clear();
  lcd_stm32h7_color(BLK, GRE);
  lcd_stm32h7_message("Saptashri Secure\nXIP Bootloader\n");
  HAL_Delay(1200U);
  lcd_stm32h7_breathout();
  lcd_stm32h7_clear();
  lcd_stm32h7_color(BLK, BLE);
}
