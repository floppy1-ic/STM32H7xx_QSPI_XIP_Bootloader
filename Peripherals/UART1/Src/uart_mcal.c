/**
  ******************************************************************************
  * @file    uart_mcal.c
  * @brief   USART1 UART MCAL implementation.
  ******************************************************************************
  */

#include "uart_mcal.h"
#include <string.h>

extern UART_HandleTypeDef huart1;

static HAL_StatusTypeDef uart_recv_byte(uint8_t *byte, uint32_t timeout_ms)
{
  return HAL_UART_Receive(&huart1, byte, 1U, timeout_ms);
}

HAL_StatusTypeDef uart_send_char(char c)
{
  return HAL_UART_Transmit(&huart1, (const uint8_t *)&c, 1U, UART_MCAL_IO_TIMEOUT_MS);
}

HAL_StatusTypeDef uart_send_string(char *str)
{
  size_t len;

  if (str == NULL)
  {
    return HAL_ERROR;
  }

  len = strlen(str);
  if (len == 0U)
  {
    return HAL_OK;
  }

  if (len > 0xFFFFU)
  {
    return HAL_ERROR;
  }

  return HAL_UART_Transmit(&huart1, (const uint8_t *)str, (uint16_t)len, UART_MCAL_IO_TIMEOUT_MS);
}

HAL_StatusTypeDef uart_recv_char(char *c)
{
  if (c == NULL)
  {
    return HAL_ERROR;
  }

  return uart_recv_byte((uint8_t *)c, HAL_MAX_DELAY);
}

uint32_t uart_recv_string(char *buff, size_t size)
{
  uint32_t count = 0U;
  char ch;
  HAL_StatusTypeDef status;

  if ((buff == NULL) || (size < 2U))
  {
    return 0U;
  }

  buff[0] = '\0';

  while (count < (size - 1U))
  {
    status = uart_recv_byte((uint8_t *)&ch, UART_MCAL_POLL_TIMEOUT_MS);
    if (status != HAL_OK)
    {
      break;
    }

    if ((ch == '\r') || (ch == '\n'))
    {
      if (count == 0U)
      {
        continue;
      }
      break;
    }

    buff[count] = ch;
    count++;
  }

  buff[count] = '\0';
  return count;
}