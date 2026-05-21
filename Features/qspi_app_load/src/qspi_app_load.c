/**
  ******************************************************************************
  * @file    qspi_app_load.c
  * @author  Sibun
  * @brief   Load application binary into external QSPI flash (UART host protocol).
  *
  * Protocol (must match tools/saptashri_flash.py):
  *   1. Host: SP (repeat)     MCU: true\r\n | false\r\n
  *   2. MCU: erase 8 MB       Host: wait EC\r\n
  *   3. MCU: WE\r\n           Host: wait WE
  *   4. Host: 'S' + 4-byte LE size   MCU: 'K' | 'N'
  *   5. Host: data chunks     MCU: Y | N per chunk
  *   6. Success: flag 0, mmap on, reset. Failure: flag 1, mmap on, return to BL idle.
  *
  * @copyright
  * Copyright (c) 2026 Sibun. All rights reserved.
  ******************************************************************************
  */

#include "qspi_app_load.h"
#include "app_shared_ram.h"
#include "qspi_flash.h"
#include "lcd_mcal.h"
#include "uart_mcal.h"
#include "main.h"

extern QSPI_HandleTypeDef hqspi;
extern UART_HandleTypeDef huart1;

#define FLASH_WRITE_CHUNK_BYTES       (256U)
/* Full W25Q64 (8 MiB) erase window before program */
#define FLASH_CLEAN_SIZE_BYTES        W25Q64_FLASH_SIZE
#define FLASH_UART_FIRST_BYTE_MS      (30000U)
#define FLASH_UART_NEXT_BYTE_MS       (100U)
#define FLASH_POST_WE_DELAY_MS        (20U)

#define FLASH_RESP_MMAP_OK            "true\r\n"
#define FLASH_RESP_MMAP_FAIL          "false\r\n"
#define FLASH_RESP_ERASE_COMPLETE     "EC\r\n"
#define FLASH_RESP_WRITE_ENABLE       "WE\r\n"
#define FLASH_SYNC_SIZE               ('S')
#define FLASH_ACK_SIZE_OK             ('K')
#define FLASH_ACK_CHUNK_OK            ('Y')
#define FLASH_NACK                    ('N')

bool qspi_app_erase_flash_sector(uint32_t sector_byte_offset)
{
  return (QSPI_Flash_EraseSector(&hqspi, sector_byte_offset) == QSPI_FLASH_OK);
}

bool qspi_app_write_flash(uint32_t byte_offset, const uint8_t *data, uint32_t len)
{
  if (data == NULL || len == 0U)
  {
    return false;
  }

  return (QSPI_Flash_Write(&hqspi, byte_offset, data, len) == QSPI_FLASH_OK);
}

static bool flash_uart_wait_tx_done(void)
{
  uint32_t tick = HAL_GetTick();

  while (HAL_UART_GetState(&huart1) != HAL_UART_STATE_READY)
  {
    if ((HAL_GetTick() - tick) > 1000U)
    {
      return false;
    }
  }

  return true;
}

static bool flash_uart_send_ack(char ack)
{
  if (uart_send_char(ack) != HAL_OK)
  {
    return false;
  }

  return flash_uart_wait_tx_done();
}

static bool flash_uart_recv_byte(uint8_t *byte, uint32_t timeout_ms)
{
  return (HAL_UART_Receive(&huart1, byte, 1U, timeout_ms) == HAL_OK);
}

static bool flash_uart_recv_exact(uint8_t *buf, uint32_t len)
{
  uint32_t i;
  uint32_t timeout_ms = FLASH_UART_FIRST_BYTE_MS;

  for (i = 0U; i < len; i++)
  {
    if (!flash_uart_recv_byte(&buf[i], timeout_ms))
    {
      return false;
    }

    timeout_ms = FLASH_UART_NEXT_BYTE_MS;
  }

  return true;
}

static bool flash_receive_image_size(uint32_t *image_size)
{
  uint8_t sync;
  uint8_t size_buf[4];

  if (!flash_uart_recv_byte(&sync, FLASH_UART_FIRST_BYTE_MS))
  {
    return false;
  }

  if (sync != (uint8_t)FLASH_SYNC_SIZE)
  {
    return false;
  }

  if (!flash_uart_recv_exact(size_buf, 4U))
  {
    return false;
  }

  *image_size = (uint32_t)size_buf[0]
                | ((uint32_t)size_buf[1] << 8)
                | ((uint32_t)size_buf[2] << 16)
                | ((uint32_t)size_buf[3] << 24);

  if ((*image_size == 0U) || (*image_size > FLASH_CLEAN_SIZE_BYTES))
  {
    return false;
  }

  return true;
}

static bool flash_cleaning(void)
{
  uint32_t offset;

  lcd_stm32h7_message("Erasing 8 MB...\n");

  for (offset = QSPI_APP_LOAD_FLASH_BASE;
       offset < (QSPI_APP_LOAD_FLASH_BASE + FLASH_CLEAN_SIZE_BYTES);
       offset += W25Q64_BLOCK_64K_SIZE)
  {
    if (QSPI_Flash_EraseBlock64K(&hqspi, offset) != QSPI_FLASH_OK)
    {
      (void)flash_uart_send_ack(FLASH_NACK);
      return false;
    }
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_3);
  }

  (void)uart_send_string((char *)FLASH_RESP_ERASE_COMPLETE);
  (void)flash_uart_wait_tx_done();
  return true;
}

static bool flash_writing(void)
{
  uint8_t chunk[FLASH_WRITE_CHUNK_BYTES];
  uint32_t flash_off = QSPI_APP_LOAD_FLASH_BASE;
  uint32_t image_size = 0U;
  uint32_t remaining;
  uint32_t chunk_len;

  (void)uart_send_string((char *)FLASH_RESP_WRITE_ENABLE);
  if (!flash_uart_wait_tx_done())
  {
    return false;
  }

  HAL_Delay(FLASH_POST_WE_DELAY_MS);

  if (!flash_receive_image_size(&image_size))
  {
    (void)flash_uart_send_ack(FLASH_NACK);
    return false;
  }

  if (!flash_uart_send_ack(FLASH_ACK_SIZE_OK))
  {
    return false;
  }

  lcd_stm32h7_message("UART program...\n");

  while (flash_off < image_size)
  {
    remaining = image_size - flash_off;
    chunk_len = (remaining >= FLASH_WRITE_CHUNK_BYTES) ? FLASH_WRITE_CHUNK_BYTES : remaining;

    if (!flash_uart_recv_exact(chunk, chunk_len))
    {
      (void)flash_uart_send_ack(FLASH_NACK);
      return false;
    }

    if (!qspi_app_write_flash(flash_off, chunk, chunk_len))
    {
      (void)flash_uart_send_ack(FLASH_NACK);
      return false;
    }

    flash_off += chunk_len;

    if (!flash_uart_send_ack(FLASH_ACK_CHUNK_OK))
    {
      return false;
    }
  }

  return true;
}

/**
  * @brief  P0-8: Abort load without reset; keep flag 1 so next boot retries load path.
  */
static void flash_load_session_abort(void)
{
  app_load_enable();
  (void)QSPI_Flash_DisableMemoryMappedMode(&hqspi);
}

/**
  * @brief  Successful program: clear load flag and reset into mmap + jump path.
  */
static void flash_load_session_complete(void)
{
  lcd_stm32h7_message("Load OK\n");
  HAL_Delay(300U);
  app_load_disable();
  (void)QSPI_Flash_EnableMemoryMappedMode(&hqspi);
  NVIC_SystemReset();
}

void qspi_new_app_load(void)
{
  lcd_stm32h7_clear();
  lcd_stm32h7_color(BLK, BLE);
  lcd_stm32h7_message("App load : Wait...\n");

  if (QSPI_Flash_DisableMemoryMappedMode(&hqspi) != QSPI_FLASH_OK)
  {
    (void)uart_send_string((char *)FLASH_RESP_MMAP_FAIL);
    (void)flash_uart_wait_tx_done();
    lcd_stm32h7_message("Mmap off fail\n");
    flash_load_session_abort();
    return;
  }

  (void)uart_send_string((char *)FLASH_RESP_MMAP_OK);
  (void)flash_uart_wait_tx_done();

  if (!flash_cleaning())
  {
    lcd_stm32h7_message("Erase failed\n");
    flash_load_session_abort();
    return;
  }

  if (!flash_writing())
  {
    lcd_stm32h7_message("Program failed\n");
    flash_load_session_abort();
    return;
  }

  flash_load_session_complete();
}
