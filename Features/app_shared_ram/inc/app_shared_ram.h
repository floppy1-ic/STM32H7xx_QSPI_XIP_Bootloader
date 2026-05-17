/**
  ******************************************************************************
  * @file    app_shared_ram.h
  * @author  Sibun
  * @brief   Bootloader write_bin flag in Backup SRAM (survives soft reset).
  *
  * @copyright
  * Copyright (c) 2026 Sibun. All rights reserved.
  ******************************************************************************
  */

#ifndef APP_SHARED_RAM_H
#define APP_SHARED_RAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* Backup SRAM flag (STM32H750) --------------------------------------------- */
#ifndef APP_SHARED_RAM_BKPSRAM_BASE
#define APP_SHARED_RAM_BKPSRAM_BASE      (0x38800000UL)  /* word0: 1=load, 0=run app */
#endif

/**
  * @brief  Set write_bin flag to 1 — bootloader runs QSPI program path.
  * @param  None
  * @retval None
  */
void app_load_enable(void);

/**
  * @brief  Set write_bin flag to 0 — normal mmap + jump path.
  * @param  None
  * @retval None
  */
void app_load_disable(void);

/**
  * @brief  Return true if write_bin flag is 1 (load new app on external QSPI).
  * @param  None
  * @retval true if flag is 1, false otherwise.
  */
bool app_load_is_enabled(void);

/**
  * @brief  Return true if write_bin flag is 0 (jump to app at XIP).
  * @param  None
  * @retval true if flag is 0, false otherwise.
  */
bool app_load_is_disabled(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SHARED_RAM_H */
