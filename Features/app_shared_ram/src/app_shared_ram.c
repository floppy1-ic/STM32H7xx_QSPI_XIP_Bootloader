/**
  ******************************************************************************
  * @file    app_shared_ram.c
  * @author  Sibun
  * @brief   write_bin flag in Backup SRAM @ 0x38800000.
  *
  * @copyright
  * Copyright (c) 2026 Sibun. All rights reserved.
  ******************************************************************************
  */

#include "app_shared_ram.h"
#include "main.h"
#include <stdbool.h>

static volatile uint32_t *App_Load_FlagPtr(void)
{
  return (volatile uint32_t *)APP_SHARED_RAM_BKPSRAM_BASE;
}

/**
  * @brief  Enable PWR backup access and BKPRAM clock (idempotent).
  */
static void App_Load_PrepareHw(void)
{
  static bool prepared = false;

  if (prepared)
  {
    return;
  }

  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_BKPRAM_CLK_ENABLE();
  prepared = true;
}

/**
  * @brief  Set write_bin flag to 1 — bootloader runs QSPI program path.
  * @param  None
  * @retval None
  */
void app_load_enable(void)
{
  App_Load_PrepareHw();
  *App_Load_FlagPtr() = 1U;
  __DSB();
}

/**
  * @brief  Set write_bin flag to 0 — normal mmap + jump path.
  * @param  None
  * @retval None
  */
void app_load_disable(void)
{
  App_Load_PrepareHw();
  *App_Load_FlagPtr() = 0U;
  __DSB();
}

/**
  * @brief  Read write_bin flag from Backup SRAM.
  * @param  None
  * @retval Flag word at APP_SHARED_RAM_BKPSRAM_BASE.
  */
static uint32_t App_Load_ReadFlag(void)
{
  App_Load_PrepareHw();
  return *App_Load_FlagPtr();
}

/**
  * @brief  Return true if write_bin flag is 1 (load new app on external QSPI).
  * @param  None
  * @retval true if flag is 1, false otherwise.
  */
bool app_load_is_enabled(void)
{
  return (App_Load_ReadFlag() == 1U);
}

/**
  * @brief  Return true if write_bin flag is 0 (jump to app at XIP).
  * @param  None
  * @retval true if flag is 0, false otherwise.
  */
bool app_load_is_disabled(void)
{
  return (App_Load_ReadFlag() == 0U);
}
