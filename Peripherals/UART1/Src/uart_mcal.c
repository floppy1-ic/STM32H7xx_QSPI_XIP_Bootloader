/**
  ******************************************************************************
  * @file    uart_mcal.c
  * @brief   USART1 UART MCAL implementation.
  ******************************************************************************
  */

#include "uart_mcal.h"
#include <string.h>

extern UART_HandleTypeDef huart1;

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

HAL_StatusTypeDef uart_recv_char_timeout(char *c, uint32_t timeout_ms)
{
  if (c == NULL)
  {
    return HAL_ERROR;
  }

  return HAL_UART_Receive(&huart1, (uint8_t *)c, 1U, timeout_ms);
}

HAL_StatusTypeDef uart_recv_char(char *c)
{
  return uart_recv_char_timeout(c, UART_MCAL_IO_TIMEOUT_MS);
}

bool uart_recv_string(char *buff, size_t size)
{
  uint32_t count = 0U;
  char ch;

  if ((buff == NULL) || (size < 3U))
  {
    return false;
  }

  buff[0] = '\0';

  while (count < (size - 1U))
  {
    if (uart_recv_char_timeout(&ch, UART_MCAL_POLL_TIMEOUT_MS) != HAL_OK)
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

  while ((count > 0U) && ((buff[count - 1U] == '\r') || (buff[count - 1U] == '\n')))
  {
    buff[count - 1U] = '\0';
    count--;
  }

  return (count == 2U) && (buff[0] == 'S') && (buff[1] == 'P');
}
