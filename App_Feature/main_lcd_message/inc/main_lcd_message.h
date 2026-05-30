/**
  ******************************************************************************
  * @file    main_lcd_message.h
  * @author  Sibun
  * @brief   Bootloader LCD messages (main path).
  *
  * @copyright
  * Copyright (c) 2026 Sibun. All rights reserved.
  ******************************************************************************
  */

#ifndef MAIN_LCD_MESSAGE_H
#define MAIN_LCD_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  Splash: init LCD, show bootloader title, breathout, clear, set blue theme.
  * @param  None
  * @retval None
  */
void main_bootloader_msg(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_LCD_MESSAGE_H */
