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

## Hardware summary (QSPI flash + TFT LCD)

### QSPI — Winbond W25Q64

| Item | Value |
|------|--------|
| **MCU** | STM32H750VBTx |
| **Peripheral** | QUADSPI (BK1, single flash) |
| **Flash IC** | **Winbond W25Q64** (8 MByte external NOR) |

| QSPI signal | MCU pin | CubeMX User Label | Alternate function | GPIO max speed |
|-------------|---------|-------------------|--------------------|----------------|
| `QUADSPI_CLK` | **PB2** | _(none)_ | `GPIO_AF9_QUADSPI` | `VERY_HIGH` |
| `QUADSPI_BK1_NCS` | **PB6** | _(none, locked)_ | `GPIO_AF10_QUADSPI` | `VERY_HIGH` |
| `QUADSPI_BK1_IO0` | **PD11** | _(none)_ | `GPIO_AF9_QUADSPI` | `VERY_HIGH` |
| `QUADSPI_BK1_IO1` | **PD12** | _(none)_ | `GPIO_AF9_QUADSPI` | `VERY_HIGH` |
| `QUADSPI_BK1_IO2` | **PE2** | _(none)_ | `GPIO_AF9_QUADSPI` | `VERY_HIGH` |
| `QUADSPI_BK1_IO3` | **PD13** | _(none, locked)_ | `GPIO_AF9_QUADSPI` | `VERY_HIGH` |

### TFT LCD — ST7735 (SPI4)

| Item | Value |
|------|--------|
| **Panel / controller** | **ST7735** family (SPI RGB TFT) |
| **MCU data bus** | **SPI4** (8-bit, software NSS; CS/DC on GPIO) |
| **Backlight** | **TIM1** channel 2 complementary output (**TIM1_CH2N** on **PE10**) |

| LCD / bus signal | MCU pin | CubeMX User Label | Mode / alternate function | GPIO max speed |
|------------------|---------|-------------------|---------------------------|----------------|
| `SPI4_MISO` | **PE5** | _(none)_ | `GPIO_AF5_SPI4` | `VERY_HIGH` |
| `SPI4_SCK` | **PE12** | _(none)_ | `GPIO_AF5_SPI4` | `VERY_HIGH` |
| `SPI4_MOSI` | **PE14** | _(none)_ | `GPIO_AF5_SPI4` | `VERY_HIGH` |
| LCD chip select (CS) | **PE11** | **`LCD_CS`** | `GPIO_Output` | `LOW` |
| Data / command (DC, RS) | **PE13** | **`LCD_WR_RS`** | `GPIO_Output` | `LOW` |
| Backlight PWM | **PE10** | _(none, locked)_ | `GPIO_AF1_TIM1` (TIM1_CH2N) | `LOW` |

---

## 1) Pin map (must match CubeMX `.ioc`)

The W25Q64 QSPI pinout is summarized in **Hardware summary → QSPI — Winbond W25Q64** above. Board-specific notes:

> **Why PB6 uses AF10 while every other pin uses AF9:**
> On STM32H750, `QUADSPI_BK1_NCS` is exposed on AF9 for `PB10` and on **AF10 for `PB6`**.
> CubeMX picks the correct AF automatically. Do not change it manually.

> Pins for `PB6` and `PD13` are kept *Locked* in the `.ioc` so a careless re-pinout cannot
> drift them back to the CubeMX defaults (`PB10` / `PA1`) which do not match this board.

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

## 7) SPI4 + TIM1 for the on-board TFT LCD

The MiniSTM32H750 carries an SPI TFT (ST7735 family) driven from **SPI4** for the data
bus and **TIM1_CH2N** for the backlight PWM. The driver under
`Per_Driver/SPI4_LCD/` accesses these through the CubeMX-generated handles
`hspi4` and `htim1`, plus two GPIO outputs for `CS` and `DC/RS`.

### 7.1) LCD pin map (must match CubeMX `.ioc`)

SPI / backlight GPIO lines are listed in **Hardware summary → TFT LCD — ST7735 (SPI4)** above. Extra nets used by the app:

| Signal            | MCU pin | CubeMX User Label | Notes |
|-------------------|---------|-------------------|--------|
| LCD `RESET`       | n/a     | n/a               | Not driven by MCU; `LCD_RST_SET` / `LCD_RST_RESET` in `lcd_app.c` are intentionally empty |
| Heartbeat LED     | **PE3** | _(none)_          | `GPIO_Output`, `LOW` speed; toggled in `main.c` `while(1)` |

> **The User Label matters.** CubeMX auto-generates macros from the User Label, so
> `LCD_CS` → `LCD_CS_Pin` / `LCD_CS_GPIO_Port` and `LCD_WR_RS` → `LCD_WR_RS_Pin` /
> `LCD_WR_RS_GPIO_Port`. The LCD driver in `Per_Driver/SPI4_LCD/Src/lcd_app.c` references
> exactly those names; renaming the label breaks the build.

### 7.2) SPI4 / TIM1 peripheral configuration

| Peripheral | Field                       | Value                          | Notes                                   |
|------------|-----------------------------|--------------------------------|-----------------------------------------|
| SPI4       | `Mode`                      | `SPI_MODE_MASTER`              | MCU is master                           |
| SPI4       | `Direction`                 | `SPI_DIRECTION_2LINES`         | Full-duplex (MISO unused but routed)    |
| SPI4       | `DataSize`                  | `SPI_DATASIZE_8BIT`            | Standard ST7735 framing                 |
| SPI4       | `NSS`                       | `SPI_NSS_SOFT`                 | CS handled by `LCD_CS` GPIO, not SPI HW |
| SPI4       | `BaudRatePrescaler`         | `÷16` (CubeMX default)         | ≈ 60 Mbit/s @ 240 MHz HCLK; lower if you see corruption |
| SPI4       | `CLKPolarity` / `CLKPhase`  | `LOW` / `1EDGE`                | ST7735 mode 0                           |
| SPI4 clock | `Spi45ClockSelection`       | `RCC_SPI45CLKSOURCE_D2PCLK1`   | Driven by APB1 (D2) per `.ioc` defaults |
| TIM1       | `Channel-PWM Generation2`   | `TIM_CHANNEL_2` CH2N           | Drives backlight via complementary out  |
| TIM1       | `ARR` (period)              | `65535` (full 16-bit)          | `LCD_SetBrightness` writes raw compare  |
| TIM1       | Start API                   | `HAL_TIMEx_PWMN_Start(...)`    | **Note `PWMN`, not `PWM`** for CH2N     |

### 7.3) Quick LCD bring-up (already wired into `main.c`)

```c
#include "lcd_mcal.h"

lcd_stm32h7_init();           /* SPI4 + TIM1 PWM + ST7735 init    */
lcd_stm32h7_backlight(100);   /* 0..100%, mapped to TIM1 compare  */
lcd_stm32h7_clear();
lcd_stm32h7_message("Hello World");
```

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
| LCD pinmap   | `SPI4_MOSI` `PE6` → `PE14`; `TIM1_CH2N` `PB0` → `PE10`; added `LCD_CS=PE11`, `LCD_WR_RS=PE13` (User Labels in CubeMX) |
| Doc refresh  | Added User Label / AF / Speed columns for QSPI and LCD pin maps          |
| Doc refresh  | Top **Hardware summary** tables: QSPI + W25Q64 header/pins, then ST7735 / SPI4 + pins |
