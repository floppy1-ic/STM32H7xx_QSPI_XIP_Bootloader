/**
  ******************************************************************************
  * @file    uart_mcal.h
  * @brief   USART1 UART MCAL (PA9 TX / PA10 RX, 115200 8N1) for STM32H7.
  *
  * CubeMX owns `huart1` and `MX_USART1_UART_Init()` in Core/Src/main.c.
  ******************************************************************************
  */

#ifndef UART_MCAL_H
#define UART_MCAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Blocking HAL timeout (ms) for one byte TX/RX. */
#define UART_MCAL_IO_TIMEOUT_MS   1000U

/** Short timeout (ms) for idle-loop UART poll (no byte available). */
#define UART_MCAL_POLL_TIMEOUT_MS  10U

/** Host load trigger bytes (see tools/saptashri_flash.py). */
#define UART_MCAL_LOAD_TRIGGER     "SP"

/**
  * @brief  Transmit one character on USART1.
  * @param  c Character to send.
  * @retval HAL_OK on success, HAL_ERROR otherwise.
  */
HAL_StatusTypeDef uart_send_char(char c);

/**
  * @brief  Transmit a null-terminated string on USART1.
  * @param  str C string; NULL is ignored.
  * @retval HAL_OK if the full string was sent, HAL_ERROR on failure or NULL.
  */
HAL_StatusTypeDef uart_send_string(char *str);

/**
  * @brief  Receive one character on USART1 (blocking).
  * @param  c Pointer to store the received byte; must not be NULL.
  * @retval HAL_OK on success, HAL_ERROR on NULL or HAL failure.
  */
HAL_StatusTypeDef uart_recv_char(char *c);

/**
  * @brief  Receive one character with a custom timeout (for polling).
  * @param  c           Pointer to store the received byte.
  * @param  timeout_ms  HAL_UART_Receive timeout in milliseconds.
  * @retval HAL_OK on success, HAL_ERROR otherwise.
  */
HAL_StatusTypeDef uart_recv_char_timeout(char *c, uint32_t timeout_ms);

/**
  * @brief  Receive characters until newline, buffer full, or per-byte poll timeout.
  *
  * Null-terminates @p buff. Returns true only when exactly two bytes were received
  * and buff[0]=='S', buff[1]=='P'; any other input is ignored (returns false).
  *
  * @param  buff  Destination buffer; must not be NULL; size >= 3 recommended.
  * @param  size  Total buffer size in bytes (including NUL).
  * @retval true if load trigger "SP" was received, false otherwise.
  */
bool uart_recv_string(char *buff, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* UART_MCAL_H */
