/**
  ******************************************************************************
  * @file    qspi_app_jump.h
  * @author  Sibun
  * @brief   Jump from bootloader to application in external QSPI (XIP) flash.
  *
  * @copyright
  * Copyright (c) 2026 Sibun. All rights reserved.
  ******************************************************************************
  */

#ifndef QSPI_APP_JUMP_H
#define QSPI_APP_JUMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* STM32H7 QSPI memory-mapped (XIP) region base ----------------------------- */
#ifndef QSPI_APP_JUMP_XIP_BASE
#define QSPI_APP_JUMP_XIP_BASE         (0x90000000UL)
#endif

typedef enum
{
  QSPI_APP_JUMP_OK = 0,
  QSPI_APP_JUMP_ERROR,
  QSPI_APP_JUMP_INVALID_VECTOR
} qspi_app_status_t;

/**
  * @brief  Check that appBaseAddress has a valid Cortex-M vector table (SP + reset).
  * @param  appBaseAddress XIP base of the application (e.g. 0x90000000).
  * @retval true if initial SP and reset handler look valid, false otherwise.
  */
bool qspi_app_is_valid(uint32_t appBaseAddress);

/**
  * @brief  Tear down IRQs/SysTick, set VTOR/MSP, and jump to the application reset handler.
  * @param  appBaseAddress XIP base of the application (e.g. 0x90000000).
  * @retval qspi_app_status_t QSPI_APP_JUMP_OK if jump succeeds; error code if validation fails.
  */
qspi_app_status_t qspi_app_jump_to_application(uint32_t appBaseAddress);

#ifdef __cplusplus
}
#endif

#endif /* QSPI_APP_JUMP_H */
