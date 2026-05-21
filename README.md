# Saptashri Secure XIP Bootloader — STM32H750VBTx

Bootloader in **128 KB internal flash** (`0x08000000`). Application image on external **Winbond W25Q64** (8 MB), executed from **XIP** `0x90000000` after memory-mapped mode.

Full design (flowcharts, memory map, checklist): **[external_bootloader_plan.html](external_bootloader_plan.html)** — open in a browser.

---

## Project layout

```
STM32H7xx_qspi_flash/
├── Core/Src/main.c                 Boot flow, LCD, flag check, idle poll
├── Features/
│   ├── app_shared_ram/             Load flag in Backup SRAM (0 / 1)
│   ├── qspi_app_load/              qspi_new_app_load() — stub today
│   └── qspi_app_jump/              Jump to app @ 0x90000000 (ready, not wired in main)
├── Peripherals/
│   ├── QSPI_Flash/                 W25Q64 driver (erase / write / mmap)
│   ├── SPI4_LCD/                   ST7735 + TIM1 backlight
│   └── UART1/                      USART1 MCAL (PB14/PB15)
├── tools/
│   ├── saptashri_flash.py          Host UART flash tool
│   └── requirements.txt
├── external_bootloader_plan.html Design document
└── STM32H750VBTX_FLASH.ld          Bootloader linker, 128 KB
```

---

## Memory map

| Region | Address | Size | Role |
|--------|---------|------|------|
| Internal FLASH | `0x08000000` … `0x0801FFFF` | 128 KB | This bootloader |
| Backup SRAM | `0x38800000` (word 0) | 4 KB | **Load flag:** `1` = program app, `0` = run app |
| External flash | `0x00000000` … `0x007FFFFF` | 8 MB | Application binary (QSPI program) |
| QSPI XIP | `0x90000000` … `0x907FFFFF` | 8 MB | Same image — CPU fetch / jump |

**Rejected for the flag:** internal flash sector and external QSPI last sector (debug / ST-LINK issues during bring-up).

---

## Load flag (`Features/app_shared_ram`)

| API | Effect |
|-----|--------|
| `app_load_enable()` | Write **1** @ `0x38800000` |
| `app_load_disable()` | Write **0** |
| `app_load_is_enabled()` | `true` if **1** |
| `app_load_is_disabled()` | `true` if **0** |

Survives **soft reset** (`NVIC_SystemReset` / `system_reset()` in `main.c`). Cleared on full power-off unless backup domain is powered.

**ST-LINK test:** Memory window @ `0x38800000` → `1` or `0`, then reset.

---

## Boot flow (as implemented in `main.c`)

1. CubeMX init (GPIO, QSPI, SPI4, TIM1, USART1).
2. LCD welcome: **Saptashri Secure / XIP Bootloader**.
3. Read flag:
   - **1** → `qspi_new_app_load()` (does not return).
   - **0** → LCD **App load: OFF / Jump to app** → mmap + validate @ `0x90000000` → jump if valid, else bootloader poll (planned; LCD + poll done today).
   - Invalid → LCD warning, `app_load_disable()`.
4. **`while(1)`:** heartbeat on **PE3**; UART idle receive — if string is **`SP`**, `app_load_enable()` then `qspi_new_app_load()`; if flag **1**, call `qspi_new_app_load()`.

### `qspi_new_app_load()` (stub)

Today (`Features/qspi_app_load`):

1. LCD: **App load : Wait...** then stub delay.
2. `app_load_disable()` → flag **0**.
3. `NVIC_SystemReset()`.

**Planned:** disable mmap → erase/write `app.bin` @ offset 0 → clear flag → mmap on → soft reset.

### Jump path when flag is 0 (planned in firmware)

1. LCD **App load: OFF / Jump to app** (done).
2. `QSPI_Flash_EnableMemoryMappedMode` → `qspi_app_is_valid(0x90000000)`.
3. **Valid** → `qspi_app_jump_to_application()` → application running.
4. **Invalid** → stay in bootloader: PE3 heartbeat, poll flag for load request.

---

## Host flash tool (`tools/`)

1. Bootloader running (PE3 heartbeat in idle).
2. `python tools/saptashri_flash.py -p COM5 app.bin` sends **`SP`+size**, then **256-byte** chunks with **ACK** handshake.
3. Image programs @ **`0x00000000`**; MCU resets with mmap → app @ **`0x90000000`**.

See **[Host flash tool — detail](#host-flash-tool--saptashri_flashpy)** below.

---

## Build (STM32CubeIDE)

- Target: **STM32H750VBTx**, project **STM32H7xx_QSPI**.
- Add include paths if a new `Features/*/inc` or `Peripherals/*/Inc` folder is added:
  - `../Features/app_shared_ram/inc`
  - `../Features/qspi_app_load/inc`
  - `../Features/qspi_app_jump/inc`
  - `../Peripherals/UART1/Inc`
- After **Generate Code**, verify `Debug/**/subdir.mk` still lists those `-I` paths (Cube sometimes regenerates makefiles without them).

---

## Hardware

| Item | Detail |
|------|--------|
| MCU | STM32H750VBTx (MiniSTM32H750 / WeAct) |
| External flash | W25Q64 on QUADSPI BK1 — see pin table below |
| LCD | ST7735 on SPI4 + TIM1 backlight — see pin table below |
| UART | USART1 **PB14/PB15**, 115200 8N1 — see pin table below |
| Heartbeat LED | **PE3** |

Pin assignments must match **`STM32H7xx_QSPI.ioc`** and `stm32h7xx_hal_msp.c`.

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

> **NCS / IO3:** CubeMX defaults are `PB10` (NCS) and `PA1` (IO3) — **wrong** for this board. Use **`PB6`** and **`PD13`** (locked in `.ioc`). Wrong pins → ReadID `0x00 0x00 0x00`.

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

> User Labels **`LCD_CS`** / **`LCD_WR_RS`** generate `LCD_CS_Pin` / `LCD_WR_RS_Pin` in `main.h` — do not rename in CubeMX without updating the LCD driver.

### USART1 (host UART / USB‑VCP)

| USART1 signal | MCU pin | Alternate function | Notes |
|---------------|---------|--------------------|--------|
| `USART1_TX` | **PB14** | `GPIO_AF4_USART1` | To USB‑serial bridge on WeAct board |
| `USART1_RX` | **PB15** | `GPIO_AF4_USART1` | 115200 8N1 |

### Other GPIO

| Signal | MCU pin | Notes |
|--------|---------|--------|
| Heartbeat LED | **PE3** | Toggled in `main.c` idle loop |

### WeAct board quick reference

| Area | WeAct / board | This project (`.ioc`) |
|------|---------------|------------------------|
| QSPI flash | W25Q64, `PB2` CLK, `PB6` NCS, `PD11`–`PD13`, `PE2` IO2 | Matches |
| LCD SPI4 | `PE5` MISO, `PE12` SCK, `PE14` MOSI | Matches |
| LCD CS / DC | `PE11` / `PE13` | User Labels `LCD_CS`, `LCD_WR_RS` |
| LCD backlight | `PE10` TIM1_CH2N | Matches |
| USART1 | Board USB‑VCP | **PB14** TX, **PB15** RX |

---

## Status summary

| Item | Status |
|------|--------|
| Bootloader @ `0x08000000` | Done |
| QSPI W25Q64 driver | Done |
| `app_shared_ram` flag API | Done |
| LCD welcome + flag display | Done |
| `qspi_new_app_load()` stub | Done |
| While-loop flag poll + reset | Done |
| USART1 `uart_mcal` | Done |
| Host UART `SP` trigger in idle loop | Done (stub load path) |
| Real QSPI program in `qspi_new_app_load` | Done |
| Jump to app when flag 0 | Done |
| Host UART EC / WE / size + Y/N chunks | Done |

---

## Host flash tool — `saptashri_flash.py`

UART programming for **W25Q64** at flash offset **`0x00000000`**.  
Bootloader enables **memory-mapped mode** and runs the app at **`0x90000000`** after reset.

**Hardware:** USART1 **PB14/PB15** (WeAct USB‑VCP), **115200 8N1**.

### Install

```bash
pip install -r tools/requirements.txt
```

### Protocol

| Step | Host | Bootloader |
|------|------|------------|
| 1 | `SP` (repeat) | Idle → `qspi_new_app_load()` |
| 2 | Wait | mmap off → `true` / `false` |
| 3 | Wait | Erase 100 KB → `EC\r\n` |
| 4 | Wait | `WE\r\n` |
| 5 | **`S`** + 4-byte LE size | **`K`** / **`N`** |
| 6 | ≤256 B chunks | **`Y`** / **`N`** per chunk |
| 7 | — | mmap on, flag 0, **reset** |

No ST-LINK required for application updates.

### Usage

```bash
python tools/saptashri_flash.py
python tools/saptashri_flash.py -p COM5 app.bin
python tools/saptashri_flash.py -p COM5 --info app.hex
```

`.hex` linked at **`0x90000000`** is converted to offset **0** automatically.

### Firmware (UART / QSPI load)

| Module | Role |
|--------|------|
| `Peripherals/UART1/` | `uart_mcal` — send/recv on USART1 |
| `Core/Src/main.c` | Idle poll: `uart_recv_string` + **`SP`** → load |
| `Features/qspi_app_load/` | Erase, UART RX, `QSPI_Flash_Write` (stub / planned) |

Internal bootloader @ `0x08000000`: flash once via CubeIDE / ST-LINK.

---

## Copyright

Copyright © 2026 Sibun. All rights reserved.
