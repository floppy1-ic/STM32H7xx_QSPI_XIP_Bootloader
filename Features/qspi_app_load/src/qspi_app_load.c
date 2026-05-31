/**
  ******************************************************************************
  * @file    qspi_app_load.c
  * @author  Sibun
  * @brief   Load application binary into external QSPI flash (UART host protocol).
  *
  * Protocol (must match tools/saptashri_flash.py):
  *   1. Host: P (repeat)           MCU: true\r\n | false\r\n
  *   2. Host: S + 4-byte LE size   MCU: K (ok) or N
  *   3. MCU: erase ceil(size/4K)+1 sectors @ 0x0  Host: wait EC\r\n
  *   4. MCU: WE\r\n                Host: wait WE
  *   5. Host: chunks (<=256 B)     MCU: Y or N per chunk
  *   6. Success: flag 0, mmap on, reset. Failure: flag 1, return to BL idle.
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
#define FLASH_MAX_IMAGE_BYTES         W25Q64_FLASH_SIZE
#define FLASH_UART_FIRST_BYTE_MS      (30000U)
#define FLASH_UART_NEXT_BYTE_MS       (100U)
#define FLASH_UART_DRAIN_GAP_MS       (5U)
#define FLASH_POST_WE_DELAY_MS        (20U)
#define FLASH_ERASE_EXTRA_SECTORS     (1U)

#define FLASH_RESP_MMAP_OK            "true\r\n"
#define FLASH_RESP_MMAP_FAIL          "false\r\n"
#define FLASH_RESP_ERASE_COMPLETE     "EC\r\n"
#define FLASH_RESP_WRITE_ENABLE       "WE\r\n"
#define FLASH_SYNC_SIZE               ('S')
#define FLASH_ACK_SIZE_OK             ('K')
#define FLASH_ACK_CHUNK_OK            ('Y')
#define FLASH_NACK                    ('N')

/**
  * @brief  Erase a single 4 KB QSPI flash sector at the given byte offset.
  */
bool qspi_app_erase_flash_sector(uint32_t sector_byte_offset)
{
  return (QSPI_Flash_EraseSector(&hqspi, sector_byte_offset) == QSPI_FLASH_OK);
}

/**
  * @brief  Program @p len bytes from @p data into QSPI flash at @p byte_offset.
  * @retval true on success; false on NULL/zero-length input or write error.
  */
bool qspi_app_write_flash(uint32_t byte_offset, const uint8_t *data, uint32_t len)
{
  if (data == NULL || len == 0U)
  {
    return false;
  }

  return (QSPI_Flash_Write(&hqspi, byte_offset, data, len) == QSPI_FLASH_OK);
}

/**
  * @brief  Block until USART1 returns to the READY state (TX drained).
  * @retval true once ready; false if it does not settle within 1 s.
  */
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

/**
  * @brief  Send a single ACK/NACK byte to the host and wait for TX to complete.
  */
static bool flash_uart_send_ack(char ack)
{
  if (uart_send_char(ack) != HAL_OK)
  {
    return false;
  }

  return flash_uart_wait_tx_done();
}

/**
  * @brief  Receive one byte from USART1 with a per-byte timeout.
  */
static bool flash_uart_recv_byte(uint8_t *byte, uint32_t timeout_ms)
{
  return (HAL_UART_Receive(&huart1, byte, 1U, timeout_ms) == HAL_OK);
}

/**
  * @brief  Discard buffered/in-flight bytes (e.g. surplus 'P' triggers) so the
  *         size handshake starts clean. Exits after one idle gap with no byte.
  */
static void flash_uart_drain_rx(void)
{
  uint8_t junk;

  while (flash_uart_recv_byte(&junk, FLASH_UART_DRAIN_GAP_MS))
  {
    /* discard */
  }
}

/**
  * @brief  Receive exactly @p len bytes: long timeout for the first byte,
  *         short per-byte timeout thereafter. Fails if any byte times out.
  */
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

/**
  * @brief  Wait for the 'S' sync byte, then read the 4-byte little-endian image
  *         size. Rejects zero or sizes larger than the W25Q64 capacity.
  */
static bool flash_receive_image_size(uint32_t *image_size)
{
  uint8_t sync = 0U;
  uint8_t size_buf[4];
  uint32_t deadline = HAL_GetTick() + FLASH_UART_FIRST_BYTE_MS;

  /* Skip leftover trigger bytes ('P') and line endings until the 'S' sync.
     The host streams 'P' until 'true', so surplus 'P' may sit in the RX FIFO. */
  while (HAL_GetTick() < deadline)
  {
    if (!flash_uart_recv_byte(&sync, FLASH_UART_NEXT_BYTE_MS))
    {
      continue;
    }

    if (sync == (uint8_t)FLASH_SYNC_SIZE)
    {
      break;
    }
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

  if ((*image_size == 0U) || (*image_size > FLASH_MAX_IMAGE_BYTES))
  {
    return false;
  }

  return true;
}

/**
  * @brief  4 KB sectors to erase: cover image + one extra sector margin.
  */
static uint32_t flash_sectors_to_erase(uint32_t image_size)
{
  uint32_t sectors_for_image =
      (image_size + W25Q64_SECTOR_SIZE - 1U) / W25Q64_SECTOR_SIZE;

  return sectors_for_image + FLASH_ERASE_EXTRA_SECTORS;
}

/**
  * @brief  Erase the sectors covering @p image_size (+1 margin) from offset 0,
  *         then send "EC\r\n". NACKs and aborts on any sector erase failure.
  */
static bool flash_cleaning(uint32_t image_size)
{
  uint32_t sector_count = flash_sectors_to_erase(image_size);
  uint32_t erase_bytes = sector_count * W25Q64_SECTOR_SIZE;
  uint32_t offset;
  uint32_t base = QSPI_APP_LOAD_FLASH_BASE;

  if (erase_bytes > W25Q64_FLASH_SIZE)
  {
    return false;
  }

  lcd_stm32h7_message("Erasing Flash...\n");

  for (offset = 0U; offset < erase_bytes; offset += W25Q64_SECTOR_SIZE)
  {
    if (QSPI_Flash_EraseSector(&hqspi, base + offset) != QSPI_FLASH_OK)
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

/**
  * @brief  Send "WE\r\n", then receive the image in <=256-byte chunks, writing
  *         each to QSPI and ACKing 'Y' per chunk ('N' + abort on any failure).
  */
static bool flash_writing(uint32_t image_size)
{
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3,0);
  uint8_t chunk[FLASH_WRITE_CHUNK_BYTES];
  uint32_t flash_off = QSPI_APP_LOAD_FLASH_BASE;
  uint32_t remaining;
  uint32_t chunk_len;

  (void)uart_send_string((char *)FLASH_RESP_WRITE_ENABLE);
  if (!flash_uart_wait_tx_done())
  {
    return false;
  }

  HAL_Delay(FLASH_POST_WE_DELAY_MS);

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

/**
  * @brief  Run one UART app-load session: mmap off -> size handshake -> erase ->
  *         program. On success clears the load flag, re-enables mmap and resets;
  *         on any failure keeps flag 1 and returns to the bootloader idle loop.
  */
void qspi_new_app_load(void)
{
  uint32_t image_size = 0U;

  lcd_stm32h7_clear();
  lcd_stm32h7_backlight(100);
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

  /* Drop surplus 'P' triggers buffered before/while the host saw 'true'. */
  flash_uart_drain_rx();

  if (!flash_receive_image_size(&image_size))
  {
    (void)flash_uart_send_ack(FLASH_NACK);
    lcd_stm32h7_message("Bad size\n");
    flash_load_session_abort();
    return;
  }

  if (!flash_uart_send_ack(FLASH_ACK_SIZE_OK))
  {
    flash_load_session_abort();
    return;
  }

  if (!flash_cleaning(image_size))
  {
    lcd_stm32h7_message("Erase failed\n");
    flash_load_session_abort();
    return;
  }

  if (!flash_writing(image_size))
  {
    lcd_stm32h7_message("Program failed\n");
    flash_load_session_abort();
    return;
  }

  flash_load_session_complete();
}
