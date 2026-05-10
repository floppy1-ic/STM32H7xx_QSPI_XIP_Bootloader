/**
  ******************************************************************************
  * @file    qspi_flash.c
  * @brief   Winbond W25Q64 external QSPI flash driver (BSP-aligned).
  ******************************************************************************
  */

#include "qspi_flash.h"

#define QSPI_FLASH_DEFAULT_TIMEOUT       HAL_QSPI_TIMEOUT_DEFAULT_VALUE
#define QSPI_FLASH_PROGRAM_TIMEOUT       1000U
#define QSPI_FLASH_ERASE_TIMEOUT         400000U
#define QSPI_FLASH_AUTOPOLL_INTERVAL     0x10U

/* Static helpers ----------------------------------------------------------- */
static QSPI_Flash_StatusTypeDef QSPI_Flash_WriteEnable(QSPI_HandleTypeDef *hqspi);
static QSPI_Flash_StatusTypeDef QSPI_Flash_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi, uint32_t Timeout);
static QSPI_Flash_StatusTypeDef QSPI_Flash_HALToStatus(HAL_StatusTypeDef halStatus);

/* ========================================================================== */
/* Public API                                                                  */
/* ========================================================================== */

QSPI_Flash_StatusTypeDef QSPI_Flash_Init(QSPI_HandleTypeDef *hqspi)
{
  uint8_t id[2] = {0U};

  if (hqspi == NULL)
  {
    return QSPI_FLASH_ERROR;
  }

  /* BSP improvement: minimal Init - rely on the prescaler / GPIO speed already set
   * by CubeMX. Sequence is Reset -> ReadID -> validate manufacturer.              */
  if (QSPI_Flash_Reset(hqspi) != QSPI_FLASH_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  if (QSPI_Flash_ReadID(hqspi, id) != QSPI_FLASH_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  if (id[0] != W25Q64_MANUFACTURER_ID)
  {
    return QSPI_FLASH_ERROR;
  }

  return QSPI_FLASH_OK;
}

QSPI_Flash_StatusTypeDef QSPI_Flash_Reset(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef cmd = {0};

  if (hqspi == NULL)
  {
    return QSPI_FLASH_ERROR;
  }

  cmd.AddressMode       = QSPI_ADDRESS_NONE;
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_NONE;
  cmd.DummyCycles       = 0U;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* BSP improvement: blind 4-line reset first to recover the chip from QPI mode
   * (e.g. soft reset of the MCU while the flash kept its prior state). Errors are
   * intentionally ignored - the chip may not even be in QPI - but it guarantees
   * the subsequent 1-line reset will land. Mirrors the vendor BSP reset path.    */
  cmd.InstructionMode = QSPI_INSTRUCTION_4_LINES;
  cmd.Instruction     = W25Q64_CMD_ENABLE_RESET;
  (void)HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT);

  cmd.Instruction = W25Q64_CMD_RESET_DEVICE;
  (void)HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT);

  HAL_Delay(1U);

  /* Standard 1-line reset (always supported regardless of prior state). */
  cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction     = W25Q64_CMD_ENABLE_RESET;
  if (HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT) != HAL_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  cmd.Instruction = W25Q64_CMD_RESET_DEVICE;
  if (HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT) != HAL_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  HAL_Delay(1U);
  return QSPI_FLASH_OK;
}

QSPI_Flash_StatusTypeDef QSPI_Flash_ReadID(QSPI_HandleTypeDef *hqspi, uint8_t *pID)
{
  QSPI_CommandTypeDef cmd = {0};
  HAL_StatusTypeDef status;

  if ((hqspi == NULL) || (pID == NULL))
  {
    return QSPI_FLASH_ERROR;
  }

  /* BSP improvement: use 0x90 Manufacturer/Device ID with a 24-bit address (0x000000).
   * Returns:
   *   pID[0] = Manufacturer ID (0xEF for Winbond)
   *   pID[1] = Device ID       (0x16 for W25Q64)                                     */
  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = W25Q64_CMD_MANUFACTURER_DEV_ID;
  cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
  cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
  cmd.Address           = 0x000000U;
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_1_LINE;
  cmd.NbData            = 2U;
  cmd.DummyCycles       = 0U;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  status = HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT);
  if (status != HAL_OK)
  {
    return QSPI_Flash_HALToStatus(status);
  }

  return QSPI_Flash_HALToStatus(HAL_QSPI_Receive(hqspi, pID, QSPI_FLASH_DEFAULT_TIMEOUT));
}

QSPI_Flash_StatusTypeDef QSPI_Flash_EraseSector(QSPI_HandleTypeDef *hqspi, uint32_t SectorAddress)
{
  QSPI_CommandTypeDef cmd = {0};
  HAL_StatusTypeDef status;

  if ((hqspi == NULL) || (SectorAddress >= W25Q64_FLASH_SIZE))
  {
    return QSPI_FLASH_ERROR;
  }

  if (QSPI_Flash_WriteEnable(hqspi) != QSPI_FLASH_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = W25Q64_CMD_SECTOR_ERASE_4K;
  cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
  cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
  cmd.Address           = SectorAddress & ~(W25Q64_SECTOR_SIZE - 1U);
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_NONE;
  cmd.DummyCycles       = 0U;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  status = HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT);
  if (status != HAL_OK)
  {
    return QSPI_Flash_HALToStatus(status);
  }

  return QSPI_Flash_AutoPollingMemReady(hqspi, QSPI_FLASH_ERASE_TIMEOUT);
}

QSPI_Flash_StatusTypeDef QSPI_Flash_EraseBlock32K(QSPI_HandleTypeDef *hqspi, uint32_t BlockAddress)
{
  QSPI_CommandTypeDef cmd = {0};
  HAL_StatusTypeDef status;

  if ((hqspi == NULL) || (BlockAddress >= W25Q64_FLASH_SIZE))
  {
    return QSPI_FLASH_ERROR;
  }

  if (QSPI_Flash_WriteEnable(hqspi) != QSPI_FLASH_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = W25Q64_CMD_BLOCK_ERASE_32K;
  cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
  cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
  cmd.Address           = BlockAddress & ~(W25Q64_BLOCK_32K_SIZE - 1U);
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_NONE;
  cmd.DummyCycles       = 0U;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  status = HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT);
  if (status != HAL_OK)
  {
    return QSPI_Flash_HALToStatus(status);
  }

  return QSPI_Flash_AutoPollingMemReady(hqspi, QSPI_FLASH_ERASE_TIMEOUT);
}

QSPI_Flash_StatusTypeDef QSPI_Flash_EraseBlock64K(QSPI_HandleTypeDef *hqspi, uint32_t BlockAddress)
{
  QSPI_CommandTypeDef cmd = {0};
  HAL_StatusTypeDef status;

  if ((hqspi == NULL) || (BlockAddress >= W25Q64_FLASH_SIZE))
  {
    return QSPI_FLASH_ERROR;
  }

  if (QSPI_Flash_WriteEnable(hqspi) != QSPI_FLASH_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = W25Q64_CMD_BLOCK_ERASE_64K;
  cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
  cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
  cmd.Address           = BlockAddress & ~(W25Q64_BLOCK_64K_SIZE - 1U);
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_NONE;
  cmd.DummyCycles       = 0U;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  status = HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT);
  if (status != HAL_OK)
  {
    return QSPI_Flash_HALToStatus(status);
  }

  return QSPI_Flash_AutoPollingMemReady(hqspi, QSPI_FLASH_ERASE_TIMEOUT);
}

QSPI_Flash_StatusTypeDef QSPI_Flash_EraseChip(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef cmd = {0};
  HAL_StatusTypeDef status;

  if (hqspi == NULL)
  {
    return QSPI_FLASH_ERROR;
  }

  if (QSPI_Flash_WriteEnable(hqspi) != QSPI_FLASH_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = W25Q64_CMD_CHIP_ERASE;
  cmd.AddressMode       = QSPI_ADDRESS_NONE;
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_NONE;
  cmd.DummyCycles       = 0U;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  status = HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT);
  if (status != HAL_OK)
  {
    return QSPI_Flash_HALToStatus(status);
  }

  return QSPI_Flash_AutoPollingMemReady(hqspi, QSPI_FLASH_ERASE_TIMEOUT);
}

QSPI_Flash_StatusTypeDef QSPI_Flash_WritePage(QSPI_HandleTypeDef *hqspi, uint32_t Address, const uint8_t *pData, uint32_t Size)
{
  QSPI_CommandTypeDef cmd = {0};
  HAL_StatusTypeDef status;
  uint32_t offsetInPage = Address % W25Q64_PAGE_SIZE;

  if ((hqspi == NULL) || (pData == NULL) || (Size == 0U) || (Size > W25Q64_PAGE_SIZE))
  {
    return QSPI_FLASH_ERROR;
  }
  if ((Address + Size) > W25Q64_FLASH_SIZE)
  {
    return QSPI_FLASH_ERROR;
  }
  if ((offsetInPage + Size) > W25Q64_PAGE_SIZE)
  {
    return QSPI_FLASH_ERROR;          /* Page boundary crossing not allowed */
  }

  if (QSPI_Flash_WriteEnable(hqspi) != QSPI_FLASH_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = W25Q64_CMD_PAGE_PROGRAM;
  cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
  cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
  cmd.Address           = Address;
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_1_LINE;
  cmd.NbData            = Size;
  cmd.DummyCycles       = 0U;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  status = HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT);
  if (status != HAL_OK)
  {
    return QSPI_Flash_HALToStatus(status);
  }

  status = HAL_QSPI_Transmit(hqspi, (uint8_t *)pData, QSPI_FLASH_DEFAULT_TIMEOUT);
  if (status != HAL_OK)
  {
    return QSPI_Flash_HALToStatus(status);
  }

  return QSPI_Flash_AutoPollingMemReady(hqspi, QSPI_FLASH_PROGRAM_TIMEOUT);
}

QSPI_Flash_StatusTypeDef QSPI_Flash_Write(QSPI_HandleTypeDef *hqspi, uint32_t Address, const uint8_t *pData, uint32_t Size)
{
  uint32_t remaining = Size;
  uint32_t addr      = Address;
  const uint8_t *src = pData;

  if ((hqspi == NULL) || (pData == NULL) || (Size == 0U))
  {
    return QSPI_FLASH_ERROR;
  }
  if ((Address + Size) > W25Q64_FLASH_SIZE)
  {
    return QSPI_FLASH_ERROR;
  }

  while (remaining > 0U)
  {
    uint32_t pageRemain = W25Q64_PAGE_SIZE - (addr % W25Q64_PAGE_SIZE);
    uint32_t chunk      = (remaining < pageRemain) ? remaining : pageRemain;

    if (QSPI_Flash_WritePage(hqspi, addr, src, chunk) != QSPI_FLASH_OK)
    {
      return QSPI_FLASH_ERROR;
    }

    addr      += chunk;
    src       += chunk;
    remaining -= chunk;
  }

  return QSPI_FLASH_OK;
}

QSPI_Flash_StatusTypeDef QSPI_Flash_Read(QSPI_HandleTypeDef *hqspi, uint32_t Address, uint8_t *pData, uint32_t Size)
{
  QSPI_CommandTypeDef cmd = {0};
  HAL_StatusTypeDef status;

  if ((hqspi == NULL) || (pData == NULL) || (Size == 0U))
  {
    return QSPI_FLASH_ERROR;
  }
  if ((Address + Size) > W25Q64_FLASH_SIZE)
  {
    return QSPI_FLASH_ERROR;
  }

  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = W25Q64_CMD_FAST_READ;
  cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
  cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
  cmd.Address           = Address;
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_1_LINE;
  cmd.NbData            = Size;
  cmd.DummyCycles       = W25Q64_DUMMY_CYCLES_FAST_READ;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  status = HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT);
  if (status != HAL_OK)
  {
    return QSPI_Flash_HALToStatus(status);
  }

  return QSPI_Flash_HALToStatus(HAL_QSPI_Receive(hqspi, pData, QSPI_FLASH_DEFAULT_TIMEOUT));
}

QSPI_Flash_StatusTypeDef QSPI_Flash_EnableMemoryMappedMode(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef cmd = {0};
  QSPI_MemoryMappedTypeDef memMappedCfg = {0};

  if (hqspi == NULL)
  {
    return QSPI_FLASH_ERROR;
  }

  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = W25Q64_CMD_FAST_READ_QUAD_IO;
  cmd.AddressMode       = QSPI_ADDRESS_4_LINES;
  cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
  cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
  cmd.AlternateBytes    = 0x00U;
  cmd.DataMode          = QSPI_DATA_4_LINES;
  cmd.DummyCycles       = W25Q64_DUMMY_CYCLES_READ_QUAD_IO;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  memMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
  memMappedCfg.TimeOutPeriod     = 0U;

  return QSPI_Flash_HALToStatus(HAL_QSPI_MemoryMapped(hqspi, &cmd, &memMappedCfg));
}

/* ========================================================================== */
/* Static helpers                                                              */
/* ========================================================================== */

static QSPI_Flash_StatusTypeDef QSPI_Flash_WriteEnable(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef cmd = {0};
  QSPI_AutoPollingTypeDef cfg = {0};
  HAL_StatusTypeDef status;

  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = W25Q64_CMD_WRITE_ENABLE;
  cmd.AddressMode       = QSPI_ADDRESS_NONE;
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_NONE;
  cmd.DummyCycles       = 0U;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  status = HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT);
  if (status != HAL_OK)
  {
    return QSPI_Flash_HALToStatus(status);
  }

  /* Poll SR1 until WEL = 1 */
  cmd.Instruction = W25Q64_CMD_READ_STATUS_REG1;
  cmd.DataMode    = QSPI_DATA_1_LINE;

  cfg.Match           = W25Q64_SR1_WEL;
  cfg.Mask            = W25Q64_SR1_WEL;
  cfg.MatchMode       = QSPI_MATCH_MODE_AND;
  cfg.StatusBytesSize = 1U;
  cfg.Interval        = QSPI_FLASH_AUTOPOLL_INTERVAL;
  cfg.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  return QSPI_Flash_HALToStatus(HAL_QSPI_AutoPolling(hqspi, &cmd, &cfg, QSPI_FLASH_DEFAULT_TIMEOUT));
}

static QSPI_Flash_StatusTypeDef QSPI_Flash_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi, uint32_t Timeout)
{
  QSPI_CommandTypeDef cmd = {0};
  QSPI_AutoPollingTypeDef cfg = {0};

  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = W25Q64_CMD_READ_STATUS_REG1;
  cmd.AddressMode       = QSPI_ADDRESS_NONE;
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_1_LINE;
  cmd.DummyCycles       = 0U;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Poll SR1 until BUSY (WIP) = 0 */
  cfg.Match           = 0U;
  cfg.Mask            = W25Q64_SR1_BUSY;
  cfg.MatchMode       = QSPI_MATCH_MODE_AND;
  cfg.StatusBytesSize = 1U;
  cfg.Interval        = QSPI_FLASH_AUTOPOLL_INTERVAL;
  cfg.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  return QSPI_Flash_HALToStatus(HAL_QSPI_AutoPolling(hqspi, &cmd, &cfg, Timeout));
}

static QSPI_Flash_StatusTypeDef QSPI_Flash_HALToStatus(HAL_StatusTypeDef halStatus)
{
  switch (halStatus)
  {
    case HAL_OK:      return QSPI_FLASH_OK;
    case HAL_BUSY:    return QSPI_FLASH_BUSY;
    case HAL_TIMEOUT: return QSPI_FLASH_TIMEOUT;
    case HAL_ERROR:
    default:          return QSPI_FLASH_ERROR;
  }
}
