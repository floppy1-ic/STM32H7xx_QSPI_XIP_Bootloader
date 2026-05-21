/**
  ******************************************************************************
  * @file    qspi_app_load.h
  * @author  Sibun
  * @brief   Load application binary into external QSPI flash.
  *
  * @copyright
  * Copyright (c) 2026 Sibun. All rights reserved.
  ******************************************************************************
  */

#ifndef QSPI_APP_LOAD_H
#define QSPI_APP_LOAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* External W25Q64 program base (chip offset, not XIP 0x90000000) */
#ifndef QSPI_APP_LOAD_FLASH_BASE
#define QSPI_APP_LOAD_FLASH_BASE       (0x00000000UL)
#endif

/**
  * @brief  UART load session (host: tools/saptashri_flash.py).
  *         On success: resets with load flag 0. On failure: returns with flag 1.
  * @param  None
  * @retval None (resets after successful program; returns to idle on failure)
  */
void qspi_new_app_load(void);

/**
  * @brief  Erase one 4 KB sector (wraps QSPI_Flash_EraseSector).
  * @param  sector_byte_offset  Byte address inside the sector (e.g. 0x00000000).
  * @retval true if erase succeeded, false otherwise.
  */
bool qspi_app_erase_flash_sector(uint32_t sector_byte_offset);

/**
  * @brief  Program bytes at external flash offset (wraps QSPI_Flash_Write).
  * @param  byte_offset  Start address on W25Q64 (must be erased).
  * @param  data         Source buffer.
  * @param  len          Number of bytes to write.
  * @retval true if write succeeded, false otherwise.
  */
bool qspi_app_write_flash(uint32_t byte_offset, const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* QSPI_APP_LOAD_H */
