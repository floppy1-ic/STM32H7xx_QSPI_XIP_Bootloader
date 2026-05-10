# QSPI Flash Driver — STM32H750VBTx + Winbond W25Q64

This module is a small, BSP-aligned QSPI driver for the **Winbond W25Q64** flash on the
**MiniSTM32H750VBTx** board. It is designed to live alongside CubeMX-generated code and
relies on the QSPI peripheral being initialized by `MX_QUADSPI_Init()`.

```
QSPI_Flash/
├── Inc/qspi_flash.h    Public API + W25Q64 commands and geometry
├── Src/qspi_flash.c    BSP-aligned implementation
└── README.md           This file
```

---

## 1) Pin map (must match CubeMX `.ioc`)

The board routes the W25Q64 to QSPI BK1 with the following fixed nets:

| Signal              | MCU pin | Alternate function | GPIO speed       |
|---------------------|---------|--------------------|------------------|
| `QUADSPI_CLK`       | **PB2** | `GPIO_AF9_QUADSPI`  | `VERY_HIGH` |
| `QUADSPI_BK1_NCS`   | **PB6** | `GPIO_AF10_QUADSPI` | `VERY_HIGH` |
| `QUADSPI_BK1_IO0`   | **PD11**| `GPIO_AF9_QUADSPI`  | `VERY_HIGH` |
| `QUADSPI_BK1_IO1`   | **PD12**| `GPIO_AF9_QUADSPI`  | `VERY_HIGH` |
| `QUADSPI_BK1_IO2`   | **PE2** | `GPIO_AF9_QUADSPI`  | `VERY_HIGH` |
| `QUADSPI_BK1_IO3`   | **PD13**| `GPIO_AF9_QUADSPI`  | `VERY_HIGH` |

> **Why PB6 uses AF10 while every other pin uses AF9:**
> On STM32H750, `QUADSPI_BK1_NCS` is exposed on AF9 for `PB10` and on **AF10 for `PB6`**.
> CubeMX picks the correct AF automatically. Do not change it manually.

### Pin pitfalls (and why we hit them)
CubeMX's *default* pinmux for QSPI BK1 selects `PB10` for `NCS` and `PA1` for `IO3`, which
**do not match** the MiniSTM32H750 board wiring. Symptom: ReadID returns `0x00 0x00 0x00`
because CS never asserts on the chip. The fix is to manually pick `PB6` and `PD13` in the
*Pinout & Configuration* view.

---

## 2) QSPI peripheral configuration (must match the BSP example `02-ExtMem_Boot`)

| `hqspi.Init.*`            | Value                                | Meaning                              |
|---------------------------|--------------------------------------|--------------------------------------|
| `ClockPrescaler`          | `2 - 1`                              | QSPI bus = kernel / (Prescaler+1)    |
| `FifoThreshold`           | `32`                                 | FIFO threshold in bytes              |
| `SampleShifting`          | `QSPI_SAMPLE_SHIFTING_HALFCYCLE`     | Sample on opposite edge for margin   |
| `FlashSize`               | `23 - 1`                             | log2(8 MB) − 1 → 22 → 8 MB           |
| `ChipSelectHighTime`      | `QSPI_CS_HIGH_TIME_8_CYCLE`          | CS high min between commands         |
| `ClockMode`               | `QSPI_CLOCK_MODE_3`                  | CPOL=1, CPHA=1 (W25Q standard)       |
| `FlashID`                 | `QSPI_FLASH_ID_1`                    | Use BK1                              |
| `DualFlash`               | `QSPI_DUALFLASH_DISABLE`             | Single chip                          |

### CubeMX defaults vs what we changed

| Field                         | CubeMX default              | Required value              | Changed? |
|-------------------------------|-----------------------------|-----------------------------|----------|
| QSPI peripheral parameters    | (matches BSP)               | (matches BSP)               | No       |
| GPIO `Maximum Output Speed`   | `LOW`                       | `VERY_HIGH`                 | **Yes**  |
| Pin: `QUADSPI_BK1_NCS`        | `PB10`                      | `PB6`                       | **Yes**  |
| Pin: `QUADSPI_BK1_IO3`        | `PA1`                       | `PD13`                      | **Yes**  |
| GPIO `Fast Mode` on `PB6`     | `Disable` (not used at this freq) | leave `Disable`       | No       |

---

## 3) Clock chain & safe QSPI speeds

### W25Q64 datasheet clock limits

| Operation                                | Max SCK frequency |
|------------------------------------------|-------------------|
| Standard SPI Read `0x03`                 | 50 MHz            |
| Fast Read `0x0B` / `0x6B` / `0xEB`       | **104 MHz**       |
| Page Program / Sector Erase / WriteSR    | 104 MHz           |

So any QSPI bus frequency from a few MHz up to **~104 MHz** is electrically safe for the
W25Q64. PCB trace quality, sample-shifting, and dummy-cycles set the *practical* upper
bound, which on this hand-soldered board is comfortable up to **~50–60 MHz**.

### STM32H750 clock chain that feeds QSPI

```
  HSE 25 MHz / HSI 64 MHz
            │
            ▼
        PLL1 (optional)
            │
            ▼
   SYSCLK ──HPRE──► HCLK / D1HCLK ──┐
                                    └──► QSPI kernel clock (D1HCLK)
                                                 │
                                          ÷ (Prescaler + 1)
                                                 ▼
                                            QSPI bus (SCK)
```

`Init.ClockPrescaler` is the value written to the prescaler field; the actual divider is
`Prescaler + 1`. So `Init.ClockPrescaler = 2 - 1 = 1` means **divide by 2**.

### Two clock scenarios you may encounter

| Scenario                              | SYSCLK | HCLK / kernel | Bus @ Prescaler 2-1 | Use this when…                       |
|---------------------------------------|--------|----------------|---------------------|--------------------------------------|
| **Bring-up (current, HSI no-PLL)**    | 64 MHz | 64 MHz         | **32 MHz**          | Verifying basic ID read / driver     |
| **Production (HSE + PLL)**            | 480 MHz| 240 MHz        | **120 MHz** ⚠ over-spec | Increase `Prescaler` to `3-1` or `4-1` to land at 60–80 MHz |

> ⚠ At a 240 MHz QSPI kernel, `Prescaler = 2 - 1` produces a 120 MHz bus — **above** the
> 104 MHz W25Q64 limit. When you switch to PLL, also bump the prescaler to keep the bus
> ≤ ~80 MHz with healthy margin.

### Recommended prescaler choices

| QSPI kernel clock (D1HCLK) | Use `Init.ClockPrescaler` | Resulting bus       |
|----------------------------|---------------------------|---------------------|
| 64 MHz (HSI, no PLL)       | `2 - 1`                   | 32 MHz (safe)       |
| 120 MHz                    | `2 - 1`                   | 60 MHz (safe)       |
| 240 MHz (PLL, full speed)  | `4 - 1` (recommended)     | 60 MHz (safe)       |
| 240 MHz                    | `3 - 1`                   | 80 MHz (still safe) |
| 240 MHz                    | `2 - 1`                   | **120 MHz (over)**  |

---

## 4) BSP improvements baked into this driver

Each item below carries a `/* BSP improvement: ... */` comment in the source.

| Area                  | Stock CubeMX behaviour                  | This driver does                                                                 |
|-----------------------|------------------------------------------|----------------------------------------------------------------------------------|
| ID command            | `0x9F` JEDEC, no address phase, 3 bytes  | `0x90` Manufacturer/Device ID, 1-line 24-bit address `0x000000`, 2 bytes         |
| Reset sequence        | Single 1-line `0x66` + `0x99`            | Blind 4-line `0x66`+`0x99` first (recovers from QPI), then 1-line `0x66`+`0x99`  |
| Init validation       | Strict 3-byte JEDEC compare              | Validate manufacturer byte (`0xEF` Winbond) only; device byte informational      |
| Quad-Enable bit write | Combined SR1+SR2 via `0x01`              | (Helper provided as `W25Q64_CMD_WRITE_STATUS_REG2 = 0x31` for SR2-only writes)   |
| Speed switching at init | n/a                                    | Removed — rely on CubeMX prescaler instead                                       |

---

## 5) Public API

```c
QSPI_Flash_StatusTypeDef QSPI_Flash_Init(QSPI_HandleTypeDef *hqspi);
QSPI_Flash_StatusTypeDef QSPI_Flash_Reset(QSPI_HandleTypeDef *hqspi);
QSPI_Flash_StatusTypeDef QSPI_Flash_ReadID(QSPI_HandleTypeDef *hqspi, uint8_t *pID);

QSPI_Flash_StatusTypeDef QSPI_Flash_EraseSector  (QSPI_HandleTypeDef *hqspi, uint32_t SectorAddress);
QSPI_Flash_StatusTypeDef QSPI_Flash_EraseBlock32K(QSPI_HandleTypeDef *hqspi, uint32_t BlockAddress);
QSPI_Flash_StatusTypeDef QSPI_Flash_EraseBlock64K(QSPI_HandleTypeDef *hqspi, uint32_t BlockAddress);
QSPI_Flash_StatusTypeDef QSPI_Flash_EraseChip    (QSPI_HandleTypeDef *hqspi);

QSPI_Flash_StatusTypeDef QSPI_Flash_WritePage(QSPI_HandleTypeDef *hqspi, uint32_t Address, const uint8_t *pData, uint32_t Size);
QSPI_Flash_StatusTypeDef QSPI_Flash_Write    (QSPI_HandleTypeDef *hqspi, uint32_t Address, const uint8_t *pData, uint32_t Size);
QSPI_Flash_StatusTypeDef QSPI_Flash_Read     (QSPI_HandleTypeDef *hqspi, uint32_t Address,       uint8_t *pData, uint32_t Size);

QSPI_Flash_StatusTypeDef QSPI_Flash_EnableMemoryMappedMode(QSPI_HandleTypeDef *hqspi);
```

### Typical bring-up sequence

```c
#include "qspi_flash.h"

extern QSPI_HandleTypeDef hqspi;
static uint8_t  jedecId[3];

if (QSPI_Flash_Init(&hqspi) != QSPI_FLASH_OK)        Error_Handler();
if (QSPI_Flash_ReadID(&hqspi, jedecId) != QSPI_FLASH_OK) Error_Handler();

/* Expected: jedecId[0] = 0xEF, jedecId[1] = 0x16, jedecId[2] = 0x00 (untouched). */
```

---

## 6) Memory map cheat-sheet

| View                       | Address range              | Notes                                  |
|----------------------------|----------------------------|----------------------------------------|
| W25Q64 internal address    | `0x00_0000` … `0x7F_FFFF`  | 8 MB linear flash address space        |
| STM32 memory-mapped (XIP)  | `0x9000_0000` … `0x907F_FFFF` | Visible only after `EnableMemoryMappedMode` |

---

## 7) SPI4 for TFT LCD (e.g. ST7735)

Many MiniSTM32H750 builds drive a small SPI TFT over **SPI4**. CubeMX generates `MX_SPI4_Init()`; keep **software NSS** so you can bit-bang `CS`/`DC`/`RST` in your display layer. The ST7735 family is sensitive to very fast SCK, so a **conservative baud prescaler** (here `÷16`) is a reliable starting point before tuning for your wiring and panel.

Basic init (paste into `spi.c` user sections or match in CubeMX *Parameter Settings*):

```c
static void MX_SPI4_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */

  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  /* SPI4 parameter configuration*/
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  /* ST7735 is sensitive to very high SPI clock; keep a conservative rate. */
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 0x0;
  hspi4.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi4.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi4.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi4.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi4.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi4.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi4.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi4.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */

}
```

After this, call `MX_SPI4_Init()` from your startup path (same style as other peripherals) and use `HAL_SPI_Transmit()` / `HAL_SPI_TransmitReceive()` from your LCD driver with explicit CS/DC control.

---

## 8) Troubleshooting matrix

| Symptom on `id[0..2]`  | Most likely cause                                    | Fix                                        |
|------------------------|------------------------------------------------------|--------------------------------------------|
| `FF FF FF`             | Bus floating (CS not asserted, wrong pin / unpowered)| Check pinmap, board power                  |
| `00 00 00`             | Bus driven low / mis-timed sample / wrong pinmap     | Check GPIO speed = `VERY_HIGH`, lower QSPI clock, verify `PB6/PD13` |
| `EF FF` / partial      | Marginal timing                                      | Lower prescaler, check `SampleShifting`    |
| `EF 16`                | Working                                              | Driver and hardware both healthy            |

---

## 9) Change log

| Date         | Change                                                                   |
|--------------|--------------------------------------------------------------------------|
| Initial      | Driver created from CubeMX HAL QSPI examples for W25Q64                  |
| BSP pass 1   | `0x90` ReadID, dual reset path (4-line + 1-line), relaxed `0xEF` check    |
| Pinmap fix   | `NCS` moved `PB10` → `PB6`; `IO3` moved `PA1` → `PD13`; all GPIO `VERY_HIGH` |
| Clean-up     | Removed `SetLowSpeed`/`SetHighSpeed`; rely on CubeMX prescaler            |
