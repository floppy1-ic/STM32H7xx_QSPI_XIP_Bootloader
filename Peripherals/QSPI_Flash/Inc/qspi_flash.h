/**
  ******************************************************************************
  * @file    qspi_flash.h
  * @author  Sibun
  * @brief   Winbond W25Q64 external QSPI flash driver (WeAct align).
  *
  * @copyright
  * Copyright (c) 2026 Sibun. All rights reserved.
  *
  * Pin map (must match CubeMX .ioc):
  *   QUADSPI_CLK      -> PB2   (AF9)
  *   QUADSPI_BK1_NCS  -> PB6   (AF10)
  *   QUADSPI_BK1_IO0  -> PD11  (AF9)
  *   QUADSPI_BK1_IO1  -> PD12  (AF9)
  *   QUADSPI_BK1_IO2  -> PE2   (AF9)
  *   QUADSPI_BK1_IO3  -> PD13  (AF9)
  ******************************************************************************
  */

#ifndef QSPI_FLASH_H
#define QSPI_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* W25Q64 memory geometry ---------------------------------------------------- */
#define W25Q64_FLASH_SIZE              0x800000U   /* 8 MiB (64 Mbit; 1 byte = 8 bits → 64/8 = 8) */
#define W25Q64_FLASH_SIZE_MBYTE        (W25Q64_FLASH_SIZE / (1024U * 1024U)) /* 8 */
#define W25Q64_FLASH_SIZE_MBIT         (W25Q64_FLASH_SIZE_MBYTE * 8U)       /* 64 */
#define W25Q64_SECTOR_SIZE             0x1000U     /* 4 KBytes            */
#define W25Q64_BLOCK_32K_SIZE          0x8000U     /* 32 KBytes           */
#define W25Q64_BLOCK_64K_SIZE          0x10000U    /* 64 KBytes           */
#define W25Q64_PAGE_SIZE               0x100U      /* 256 Bytes           */
#define W25Q64_SECTOR_COUNT            (W25Q64_FLASH_SIZE / W25Q64_SECTOR_SIZE)
#define W25Q64_PAGE_COUNT              (W25Q64_FLASH_SIZE / W25Q64_PAGE_SIZE)

/* Expected JEDEC values for the W25Q64 ------------------------------------- */
#define W25Q64_MANUFACTURER_ID         0xEFU       /* Winbond                                   */
#define W25Q64_DEVICE_ID               0x16U       /* Returned by 0x90 for W25Q64               */
#define W25Q64_JEDEC_ID                0xEF4017U   /* Returned by 0x9F (kept for reference)     */

/* W25Q64 command set (WeAct align) ----------------------------------------- */
#define W25Q64_CMD_WRITE_ENABLE        0x06U
#define W25Q64_CMD_WRITE_DISABLE       0x04U
#define W25Q64_CMD_READ_STATUS_REG1    0x05U
#define W25Q64_CMD_READ_STATUS_REG2    0x35U
#define W25Q64_CMD_READ_STATUS_REG3    0x15U
#define W25Q64_CMD_WRITE_STATUS_REG1   0x01U   /* Legacy combined SR1+SR2 write              */
#define W25Q64_CMD_WRITE_STATUS_REG2   0x31U   /* Dedicated SR2 write (datasheet)            */
#define W25Q64_CMD_WRITE_STATUS_REG3   0x11U
#define W25Q64_CMD_READ_DATA           0x03U
#define W25Q64_CMD_FAST_READ           0x0BU
#define W25Q64_CMD_FAST_READ_QUAD_IO   0xEBU
#define W25Q64_CMD_PAGE_PROGRAM        0x02U
#define W25Q64_CMD_QUAD_PAGE_PROGRAM   0x32U
#define W25Q64_CMD_SECTOR_ERASE_4K     0x20U
#define W25Q64_CMD_BLOCK_ERASE_32K     0x52U
#define W25Q64_CMD_BLOCK_ERASE_64K     0xD8U
#define W25Q64_CMD_CHIP_ERASE          0xC7U
#define W25Q64_CMD_MANUFACTURER_DEV_ID 0x90U   /* Manufacturer/Device ID read (ReadID)       */
#define W25Q64_CMD_JEDEC_ID            0x9FU   /* Legacy 3-byte JEDEC (not used by ReadID)    */
#define W25Q64_CMD_ENABLE_RESET        0x66U
#define W25Q64_CMD_RESET_DEVICE        0x99U
#define W25Q64_CMD_ENTER_QPI_MODE      0x38U
#define W25Q64_CMD_EXIT_QPI_MODE       0xFFU

/* Status register bits ------------------------------------------------------ */
#define W25Q64_SR1_BUSY                0x01U   /* Write-In-Progress (WIP) */
#define W25Q64_SR1_WEL                 0x02U   /* Write Enable Latch      */
#define W25Q64_SR2_QE                  0x02U   /* Quad Enable             */

/* Dummy cycles for Fast Read commands -------------------------------------- */
#define W25Q64_DUMMY_CYCLES_FAST_READ      8U   /* 0x0B — unchanged */
#define W25Q64_MMAP_CONTINUOUS_READ_ALT    0xFFU   /* H750_BootFromExtFlash CSP_QSPI */
#define W25Q64_DUMMY_CYCLES_READ_QUAD_IO   4U      /* 0xEB mmap */

typedef enum
{
  QSPI_FLASH_OK = 0x00U,
  QSPI_FLASH_ERROR,
  QSPI_FLASH_BUSY,
  QSPI_FLASH_TIMEOUT
} QSPI_Flash_StatusTypeDef;

/**
  * @brief  Reset the flash, verify Winbond manufacturer ID, and set SR2.QE if needed.
  *         Quad Enable is required for quad I/O program/read and 0xEB memory-mapped mode.
  * @param  hqspi Pointer to the initialized QSPI handle.
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK on success, error code otherwise.
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_Init(QSPI_HandleTypeDef *hqspi);

/**
  * @brief  Recover the flash from any prior state (QPI or SPI) and software reset it.
  *         Issues a blind 4-line Enable-Reset/Reset-Device first to escape QPI, then the
  *         standard 1-line Enable-Reset/Reset-Device sequence.
  * @param  hqspi Pointer to the initialized QSPI handle.
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK on success, error code otherwise.
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_Reset(QSPI_HandleTypeDef *hqspi);

/**
  * @brief  Read Manufacturer/Device ID using command 0x90 with a 24-bit address.
  *         0x90 + addr 0x000000 is more reliable than legacy 0x9F when the chip might
  *         still be in a Quad/QPI state. Returns 2 bytes:
  *           pID[0] = Manufacturer ID (expected 0xEF for Winbond)
  *           pID[1] = Device ID       (expected 0x16 for W25Q64)
  * @param  hqspi Pointer to the initialized QSPI handle.
  * @param  pID   Pointer to a buffer of at least 2 bytes.
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK on success, error code otherwise.
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_ReadID(QSPI_HandleTypeDef *hqspi, uint8_t *pID);

/**
  * @brief  Read JEDEC ID (0x9F) and compute capacity for standard SPI NOR parts.
  *         Density byte (3rd byte) is decoded as size_bytes = 2^N (JEDEC-style; Winbond W25Q, etc.).
  *         W25Q64: N=23 → 8 MiB, 64 Mibit (64 Mibit ÷ 8 = 8 MiB).
  * @param  hqspi      Initialized QSPI handle.
  * @param  pSizeMByte Whole mebibytes: size_bytes / (1024²); 0 if capacity < 1 MiB.
  * @param  pSizeMbit  Mebibits: (size_bytes × 8) / (1024²) (e.g. 64 for W25Q64).
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK, or QSPI_FLASH_ERROR (NULL ptr / bad density / bus error).
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_GetSize(QSPI_HandleTypeDef *hqspi, uint32_t *pSizeMByte, uint32_t *pSizeMbit);

/**
  * @brief  Read 3-byte JEDEC ID (Manufacturer, Memory Type, Capacity) using command 0x9F.
  * @param  hqspi   Initialized QSPI handle.
  * @param  pJedec  Buffer for 3 bytes: [0]=Manufacturer, [1]=Type, [2]=density code (2^N bytes).
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK on success.
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_ReadJEDEC(QSPI_HandleTypeDef *hqspi, uint8_t *pJedec);

/**
  * @brief  Erase a single 4 KB sector containing the given address.
  * @param  hqspi         Pointer to the initialized QSPI handle.
  * @param  SectorAddress Byte address inside the sector to erase (auto-aligned to 4 KB).
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK on success, error code otherwise.
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_EraseSector(QSPI_HandleTypeDef *hqspi, uint32_t SectorAddress);

/**
  * @brief  Erase a 32 KB block containing the given address.
  * @param  hqspi        Pointer to the initialized QSPI handle.
  * @param  BlockAddress Byte address inside the block to erase (auto-aligned to 32 KB).
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK on success, error code otherwise.
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_EraseBlock32K(QSPI_HandleTypeDef *hqspi, uint32_t BlockAddress);

/**
  * @brief  Erase a 64 KB block containing the given address.
  * @param  hqspi        Pointer to the initialized QSPI handle.
  * @param  BlockAddress Byte address inside the block to erase (auto-aligned to 64 KB).
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK on success, error code otherwise.
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_EraseBlock64K(QSPI_HandleTypeDef *hqspi, uint32_t BlockAddress);

/**
  * @brief  Erase the entire 8 MB flash (can take ~100 s on a W25Q64).
  * @param  hqspi Pointer to the initialized QSPI handle.
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK on success, error code otherwise.
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_EraseChip(QSPI_HandleTypeDef *hqspi);

/**
  * @brief  Program up to one 256-byte page; the write must not cross a page boundary.
  * @param  hqspi   Pointer to the initialized QSPI handle.
  * @param  Address Start address (must be erased; (Address % 256) + Size <= 256).
  * @param  pData   Pointer to the source buffer.
  * @param  Size    Number of bytes to write (1..256).
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK on success, error code otherwise.
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_WritePage(QSPI_HandleTypeDef *hqspi, uint32_t Address, const uint8_t *pData, uint32_t Size);

/**
  * @brief  Program any size buffer; internally split into page-aligned WritePage calls.
  * @param  hqspi   Pointer to the initialized QSPI handle.
  * @param  Address Start address; the destination range must already be erased.
  * @param  pData   Pointer to the source buffer.
  * @param  Size    Number of bytes to write.
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK on success, error code otherwise.
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_Write(QSPI_HandleTypeDef *hqspi, uint32_t Address, const uint8_t *pData, uint32_t Size);

/**
  * @brief  Read a buffer from flash using the Fast Read (0x0B) command in indirect mode.
  * @param  hqspi   Pointer to the initialized QSPI handle.
  * @param  Address Start address inside the flash (must be < 8 MB).
  * @param  pData   Pointer to the destination buffer.
  * @param  Size    Number of bytes to read.
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK on success, error code otherwise.
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_Read(QSPI_HandleTypeDef *hqspi, uint32_t Address, uint8_t *pData, uint32_t Size);

/**
  * @brief  Enable memory-mapped mode (Quad I/O Fast Read 0xEB) - flash visible at 0x90000000.
  *         Call QSPI_Flash_Init first so SR2.QE is set.
  * @param  hqspi Pointer to the initialized QSPI handle.
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK on success, error code otherwise.
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_EnableMemoryMappedMode(QSPI_HandleTypeDef *hqspi);

/**
  * @brief  Leave memory-mapped mode and return QSPI to indirect (command) mode.
  *         Required before erase/program or QSPI_Flash_Read indirect transfers if
  *         QSPI_Flash_EnableMemoryMappedMode was used. Wraps HAL_QSPI_Abort.
  * @param  hqspi Pointer to the initialized QSPI handle.
  * @retval QSPI_Flash_StatusTypeDef QSPI_FLASH_OK on success, error code otherwise.
  */
QSPI_Flash_StatusTypeDef QSPI_Flash_DisableMemoryMappedMode(QSPI_HandleTypeDef *hqspi);

#ifdef __cplusplus
}
#endif

#endif /* QSPI_FLASH_H */
