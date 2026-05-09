/**
  ******************************************************************************
  * @file    qspi_flash.h
  * @brief   Winbond W25Q64 external QSPI flash driver.
  ******************************************************************************
  */

#ifndef QSPI_FLASH_H
#define QSPI_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* W25Q64 memory geometry ---------------------------------------------------- */
#define W25Q64_FLASH_SIZE              0x800000U   /* 8 MBytes / 64 Mbits */
#define W25Q64_SECTOR_SIZE             0x1000U     /* 4 KBytes */
#define W25Q64_BLOCK_32K_SIZE          0x8000U     /* 32 KBytes */
#define W25Q64_BLOCK_64K_SIZE          0x10000U    /* 64 KBytes */
#define W25Q64_PAGE_SIZE               0x100U      /* 256 Bytes */
#define W25Q64_SECTOR_COUNT            (W25Q64_FLASH_SIZE / W25Q64_SECTOR_SIZE)
#define W25Q64_PAGE_COUNT              (W25Q64_FLASH_SIZE / W25Q64_PAGE_SIZE)
#define W25Q64_JEDEC_ID                0xEF4017U

/* W25Q64 command set -------------------------------------------------------- */
#define W25Q64_CMD_WRITE_ENABLE        0x06U
#define W25Q64_CMD_WRITE_DISABLE       0x04U
#define W25Q64_CMD_READ_STATUS_REG1    0x05U
#define W25Q64_CMD_READ_STATUS_REG2    0x35U
#define W25Q64_CMD_WRITE_STATUS_REG    0x01U
#define W25Q64_CMD_READ_DATA           0x03U
#define W25Q64_CMD_FAST_READ           0x0BU
#define W25Q64_CMD_FAST_READ_QUAD_IO   0xEBU
#define W25Q64_CMD_PAGE_PROGRAM        0x02U
#define W25Q64_CMD_QUAD_PAGE_PROGRAM   0x32U
#define W25Q64_CMD_SECTOR_ERASE_4K     0x20U
#define W25Q64_CMD_BLOCK_ERASE_32K     0x52U
#define W25Q64_CMD_BLOCK_ERASE_64K     0xD8U
#define W25Q64_CMD_CHIP_ERASE          0xC7U
#define W25Q64_CMD_JEDEC_ID            0x9FU
#define W25Q64_CMD_ENABLE_RESET        0x66U
#define W25Q64_CMD_RESET_DEVICE        0x99U

/* Status register bits ------------------------------------------------------ */
#define W25Q64_SR1_BUSY                0x01U
#define W25Q64_SR1_WEL                 0x02U
#define W25Q64_SR2_QE                  0x02U

typedef enum
{
  QSPI_FLASH_OK = 0x00U,
  QSPI_FLASH_ERROR,
  QSPI_FLASH_BUSY,
  QSPI_FLASH_TIMEOUT
} QSPI_Flash_StatusTypeDef;

QSPI_Flash_StatusTypeDef QSPI_Flash_Init(QSPI_HandleTypeDef *hqspi);
QSPI_Flash_StatusTypeDef QSPI_Flash_Reset(QSPI_HandleTypeDef *hqspi);
QSPI_Flash_StatusTypeDef QSPI_Flash_ReadID(QSPI_HandleTypeDef *hqspi, uint8_t *pID);

QSPI_Flash_StatusTypeDef QSPI_Flash_EraseSector(QSPI_HandleTypeDef *hqspi, uint32_t SectorAddress);
QSPI_Flash_StatusTypeDef QSPI_Flash_EraseBlock32K(QSPI_HandleTypeDef *hqspi, uint32_t BlockAddress);
QSPI_Flash_StatusTypeDef QSPI_Flash_EraseBlock64K(QSPI_HandleTypeDef *hqspi, uint32_t BlockAddress);
QSPI_Flash_StatusTypeDef QSPI_Flash_EraseChip(QSPI_HandleTypeDef *hqspi);

QSPI_Flash_StatusTypeDef QSPI_Flash_WritePage(QSPI_HandleTypeDef *hqspi, uint32_t Address, const uint8_t *pData, uint32_t Size);
QSPI_Flash_StatusTypeDef QSPI_Flash_Write(QSPI_HandleTypeDef *hqspi, uint32_t Address, const uint8_t *pData, uint32_t Size);
QSPI_Flash_StatusTypeDef QSPI_Flash_Read(QSPI_HandleTypeDef *hqspi, uint32_t Address, uint8_t *pData, uint32_t Size);

QSPI_Flash_StatusTypeDef QSPI_Flash_EnableMemoryMappedMode(QSPI_HandleTypeDef *hqspi);

#ifdef __cplusplus
}
#endif

#endif /* QSPI_FLASH_H */
