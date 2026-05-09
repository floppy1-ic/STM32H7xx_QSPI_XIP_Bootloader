/**
  ******************************************************************************
  * @file    qspi_flash.c
  * @brief   Winbond W25Q64 external QSPI flash driver.
  ******************************************************************************
  */

#include "qspi_flash.h"

#define QSPI_FLASH_DEFAULT_TIMEOUT       HAL_QSPI_TIMEOUT_DEFAULT_VALUE
#define QSPI_FLASH_PROGRAM_TIMEOUT       1000U
#define QSPI_FLASH_ERASE_TIMEOUT         400000U
#define QSPI_FLASH_AUTOPOLL_INTERVAL     0x10U

static QSPI_Flash_StatusTypeDef QSPI_Flash_WriteEnable(QSPI_HandleTypeDef *hqspi);
static QSPI_Flash_StatusTypeDef QSPI_Flash_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi, uint32_t Timeout);
static QSPI_Flash_StatusTypeDef QSPI_Flash_EnableQuadMode(QSPI_HandleTypeDef *hqspi);
static QSPI_Flash_StatusTypeDef QSPI_Flash_ReadStatusRegister(QSPI_HandleTypeDef *hqspi, uint8_t Instruction, uint8_t *pStatus);
static QSPI_Flash_StatusTypeDef QSPI_Flash_HALToStatus(HAL_StatusTypeDef halStatus);

QSPI_Flash_StatusTypeDef QSPI_Flash_Init(QSPI_HandleTypeDef *hqspi)
{
  uint8_t id[3] = {0U};
  uint32_t jedecId;

  if (hqspi == NULL)
  {
    return QSPI_FLASH_ERROR;
  }

  if (QSPI_Flash_Reset(hqspi) != QSPI_FLASH_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  if (QSPI_Flash_ReadID(hqspi, id) != QSPI_FLASH_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  jedecId = ((uint32_t)id[0] << 16U) | ((uint32_t)id[1] << 8U) | id[2];
  if (jedecId != W25Q64_JEDEC_ID)
  {
    return QSPI_FLASH_ERROR;
  }

  return QSPI_Flash_EnableQuadMode(hqspi);
}

QSPI_Flash_StatusTypeDef QSPI_Flash_Reset(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef cmd = {0};
  HAL_StatusTypeDef status;

  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.AddressMode       = QSPI_ADDRESS_NONE;
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_NONE;
  cmd.DummyCycles       = 0U;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  cmd.Instruction = W25Q64_CMD_ENABLE_RESET;
  status = HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT);
  if (status != HAL_OK)
  {
    return QSPI_Flash_HALToStatus(status);
  }

  cmd.Instruction = W25Q64_CMD_RESET_DEVICE;
  status = HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT);
  if (status != HAL_OK)
  {
    return QSPI_Flash_HALToStatus(status);
  }

  HAL_Delay(1U);
  return QSPI_Flash_AutoPollingMemReady(hqspi, QSPI_FLASH_DEFAULT_TIMEOUT);
}

QSPI_Flash_StatusTypeDef QSPI_Flash_ReadID(QSPI_HandleTypeDef *hqspi, uint8_t *pID)
{
  QSPI_CommandTypeDef cmd = {0};
  HAL_StatusTypeDef status;

  if ((hqspi == NULL) || (pID == NULL))
  {
    return QSPI_FLASH_ERROR;
  }

  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = W25Q64_CMD_JEDEC_ID;
  cmd.AddressMode       = QSPI_ADDRESS_NONE;
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_1_LINE;
  cmd.NbData            = 3U;
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

  if (SectorAddress >= W25Q64_FLASH_SIZE)
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

  if (BlockAddress >= W25Q64_FLASH_SIZE)
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

  if (BlockAddress >= W25Q64_FLASH_SIZE)
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

  if ((pData == NULL) || (Size == 0U) || (Size > W25Q64_PAGE_SIZE) || ((Address + Size) > W25Q64_FLASH_SIZE))
  {
    return QSPI_FLASH_ERROR;
  }

  if (((Address & (W25Q64_PAGE_SIZE - 1U)) + Size) > W25Q64_PAGE_SIZE)
  {
    return QSPI_FLASH_ERROR;
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
  uint32_t pageRemaining;
  uint32_t chunkSize;

  if ((pData == NULL) || (Size == 0U) || ((Address + Size) > W25Q64_FLASH_SIZE))
  {
    return QSPI_FLASH_ERROR;
  }

  while (Size > 0U)
  {
    pageRemaining = W25Q64_PAGE_SIZE - (Address % W25Q64_PAGE_SIZE);
    chunkSize = (Size < pageRemaining) ? Size : pageRemaining;

    if (QSPI_Flash_WritePage(hqspi, Address, pData, chunkSize) != QSPI_FLASH_OK)
    {
      return QSPI_FLASH_ERROR;
    }

    Address += chunkSize;
    pData += chunkSize;
    Size -= chunkSize;
  }

  return QSPI_FLASH_OK;
}

QSPI_Flash_StatusTypeDef QSPI_Flash_Read(QSPI_HandleTypeDef *hqspi, uint32_t Address, uint8_t *pData, uint32_t Size)
{
  QSPI_CommandTypeDef cmd = {0};
  HAL_StatusTypeDef status;

  if ((pData == NULL) || (Size == 0U) || ((Address + Size) > W25Q64_FLASH_SIZE))
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
  cmd.DummyCycles       = 8U;
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
  QSPI_MemoryMappedTypeDef cfg = {0};

  if (QSPI_Flash_EnableQuadMode(hqspi) != QSPI_FLASH_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  cmd.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction        = W25Q64_CMD_FAST_READ_QUAD_IO;
  cmd.AddressMode        = QSPI_ADDRESS_4_LINES;
  cmd.AddressSize        = QSPI_ADDRESS_24_BITS;
  cmd.AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
  cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
  cmd.AlternateBytes     = 0xFFU;
  cmd.DataMode           = QSPI_DATA_4_LINES;
  cmd.DummyCycles        = 6U;
  cmd.DdrMode            = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;

  cfg.TimeOutPeriod     = 0U;
  cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

  return QSPI_Flash_HALToStatus(HAL_QSPI_MemoryMapped(hqspi, &cmd, &cfg));
}

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

  cmd.Instruction = W25Q64_CMD_READ_STATUS_REG1;
  cmd.DataMode = QSPI_DATA_1_LINE;

  cfg.Match = W25Q64_SR1_WEL;
  cfg.Mask = W25Q64_SR1_WEL;
  cfg.MatchMode = QSPI_MATCH_MODE_AND;
  cfg.StatusBytesSize = 1U;
  cfg.Interval = QSPI_FLASH_AUTOPOLL_INTERVAL;
  cfg.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

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

  cfg.Match = 0U;
  cfg.Mask = W25Q64_SR1_BUSY;
  cfg.MatchMode = QSPI_MATCH_MODE_AND;
  cfg.StatusBytesSize = 1U;
  cfg.Interval = QSPI_FLASH_AUTOPOLL_INTERVAL;
  cfg.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

  return QSPI_Flash_HALToStatus(HAL_QSPI_AutoPolling(hqspi, &cmd, &cfg, Timeout));
}

static QSPI_Flash_StatusTypeDef QSPI_Flash_EnableQuadMode(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef cmd = {0};
  uint8_t sr1;
  uint8_t sr2;
  uint8_t statusRegs[2];
  HAL_StatusTypeDef status;

  if (QSPI_Flash_ReadStatusRegister(hqspi, W25Q64_CMD_READ_STATUS_REG1, &sr1) != QSPI_FLASH_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  if (QSPI_Flash_ReadStatusRegister(hqspi, W25Q64_CMD_READ_STATUS_REG2, &sr2) != QSPI_FLASH_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  if ((sr2 & W25Q64_SR2_QE) != 0U)
  {
    return QSPI_FLASH_OK;
  }

  if (QSPI_Flash_WriteEnable(hqspi) != QSPI_FLASH_OK)
  {
    return QSPI_FLASH_ERROR;
  }

  statusRegs[0] = sr1;
  statusRegs[1] = sr2 | W25Q64_SR2_QE;

  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = W25Q64_CMD_WRITE_STATUS_REG;
  cmd.AddressMode       = QSPI_ADDRESS_NONE;
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

  status = HAL_QSPI_Transmit(hqspi, statusRegs, QSPI_FLASH_DEFAULT_TIMEOUT);
  if (status != HAL_OK)
  {
    return QSPI_Flash_HALToStatus(status);
  }

  return QSPI_Flash_AutoPollingMemReady(hqspi, QSPI_FLASH_PROGRAM_TIMEOUT);
}

static QSPI_Flash_StatusTypeDef QSPI_Flash_ReadStatusRegister(QSPI_HandleTypeDef *hqspi, uint8_t Instruction, uint8_t *pStatus)
{
  QSPI_CommandTypeDef cmd = {0};
  HAL_StatusTypeDef status;

  if (pStatus == NULL)
  {
    return QSPI_FLASH_ERROR;
  }

  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = Instruction;
  cmd.AddressMode       = QSPI_ADDRESS_NONE;
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_1_LINE;
  cmd.NbData            = 1U;
  cmd.DummyCycles       = 0U;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  status = HAL_QSPI_Command(hqspi, &cmd, QSPI_FLASH_DEFAULT_TIMEOUT);
  if (status != HAL_OK)
  {
    return QSPI_Flash_HALToStatus(status);
  }

  return QSPI_Flash_HALToStatus(HAL_QSPI_Receive(hqspi, pStatus, QSPI_FLASH_DEFAULT_TIMEOUT));
}

static QSPI_Flash_StatusTypeDef QSPI_Flash_HALToStatus(HAL_StatusTypeDef halStatus)
{
  switch (halStatus)
  {
    case HAL_OK:
      return QSPI_FLASH_OK;
    case HAL_BUSY:
      return QSPI_FLASH_BUSY;
    case HAL_TIMEOUT:
      return QSPI_FLASH_TIMEOUT;
    case HAL_ERROR:
    default:
      return QSPI_FLASH_ERROR;
  }
}
