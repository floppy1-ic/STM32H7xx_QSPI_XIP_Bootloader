/**
  ******************************************************************************
  * @file    uart_mcal.h
  * @brief   USART1 UART MCAL (PB14 TX / PB15 RX, 115200 8N1) for STM32H7.
  *
  * CubeMX owns `huart1` and `MX_USART1_UART_Init()` in Core/Src/main.c.
  ******************************************************************************
  */

#ifndef UART_MCAL_H
#define UART_MCAL_H

#include <stddef.h>
#include <stdint.h>
#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Blocking HAL timeout (ms) for TX. */
#define UART_MCAL_IO_TIMEOUT_MS   1000U

/** Per-byte timeout (ms) for uart_recv_string poll (does not block forever). */
#define UART_MCAL_POLL_TIMEOUT_MS  10U

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
  * @brief  Receive one byte on USART1 (blocking, HAL_UART_Receive with HAL_MAX_DELAY).
  * @param  c  Pointer to store the received byte; must not be NULL.
  * @retval HAL_OK on success, HAL_ERROR on NULL or HAL failure.
  */
HAL_StatusTypeDef uart_recv_char(char *c);

/**
  * @brief  Poll RX for a line (non-blocking between calls; UART_MCAL_POLL_TIMEOUT_MS per byte).
  *         Use uart_recv_char() when you must block until one byte arrives.
  * @param  buff  Destination buffer (NUL-terminated).
  * @param  size  Buffer size in bytes (including NUL).
  * @retval Number of characters received (excluding NUL), or 0 if none.
  */
uint32_t uart_recv_string(char *buff, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* UART_MCAL_H */
