/**
  ******************************************************************************
  * @file    qspi_app_load.c
  * @author  Sibun
  * @brief   Load application binary into external QSPI flash.
  *
  * @copyright
  * Copyright (c) 2026 Sibun. All rights reserved.
  ******************************************************************************
  */

#include "qspi_app_load.h"
#include "app_shared_ram.h"
#include "lcd_mcal.h"
#include "main.h"

/**
  * @brief  Run app-load session (program external flash when implemented).
  * @param  None
  * @retval None (does not return; soft reset after stub or real load).
  */
void qspi_new_app_load(void)
{
  lcd_stm32h7_clear();
  lcd_stm32h7_color(BLK, BLE);
  lcd_stm32h7_message("App load : Wait...\n");

  /* TODO: QSPI_Flash_DisableMemoryMappedMode, erase/write app.bin @ 0 */
  HAL_Delay(QSPI_APP_LOAD_STUB_DISPLAY_MS);

  app_load_disable();
  NVIC_SystemReset();
}
