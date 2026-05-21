# Saptashri Secure XIP Bootloader — STM32H750VBTx

Bootloader in **128 KB internal flash** (`0x08000000`). Application image on external **Winbond W25Q64** (8 MB), executed from **XIP** `0x90000000` after memory-mapped mode.

**Status:** Bootloader bring-up is **complete** — QSPI init, UART program, indirect + mmap vector reads, and jump handoff are verified. The **application** (e.g. Arundhati linked @ `0x90000000`) is a **separate project**; after jump, startup/cache/MPU in that app must be correct.

Full design (flowcharts, memory map, checklist): **[external_bootloader_plan.html](external_bootloader_plan.html)** — open in a browser.

---

## Project layout

```
STM32H7xx_qspi_flash/
├── Core/Src/main.c                 Boot flow, LCD, flag check, idle poll
├── Features/
│   ├── app_shared_ram/             Load flag in Backup SRAM (0 / 1)
│   ├── qspi_app_load/              UART program session (qspi_new_app_load)
│   └── qspi_app_jump/              Validate + jump to app @ 0x90000000
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
| `app_load_flag_sanitize()` | If word is not **0** or **1**, write **0** (cold BKPSRAM garbage) |
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
3. `app_load_flag_sanitize()` — invalid BKPSRAM word → **0**.
4. `QSPI_Flash_Init()` — reset W25Q, ID check, **Quad Enable (SR2.QE)**. On fail → LCD **`QSPI init fail`** → idle.
5. Branch on flag:
   - **1** → `qspi_new_app_load()` (UART program; see below).
   - **0** → `qspi_app_is_valid_at_flash()` (indirect `0x0B` read @ offset 0). If invalid → LCD **`No valid app`**. If valid → mmap; on mmap fail → **`Mmap fail`**. Else cache cleanup → `qspi_app_jump_to_application(0x90000000)`; if jump returns → mmap off → idle.
6. **`while(1)`:** PE3 heartbeat; `uart_recv_string` — if **`SP`** → `app_load_enable()` + `qspi_new_app_load()`.

### `qspi_new_app_load()` (UART program)

Matches `tools/saptashri_flash.py`:

1. Mmap off → host sees `true\r\n` / `false\r\n`.
2. Erase **full 8 MB** W25Q64 @ offset 0 (**128 × 64 KB** block erases) → `EC\r\n` (allow **~1–3 min**; host timeout **300 s**).
3. `WE\r\n` → host sends **`S`** + 4-byte LE size → **`K`** / **`N`** (max image size = 8 MB).
4. Data in **256-byte** chunks → **`Y`** / **`N`** per chunk.
5. **Success:** LCD **Load OK** → flag **0** → mmap on → **reset** → boot tries jump.
6. **Failure:** flag stays **1**, **mmap off**, **return** to idle (no reset; host sees **`N`** or **EC** timeout).

### LCD messages (boot / load)

| Message | Meaning |
|---------|--------|
| `QSPI init fail` | `QSPI_Flash_Init` failed |
| `No valid app` | No valid vector table in external flash |
| `Mmap fail` | Valid vectors but mmap could not start |
| `Jump to QSPI app` | Handoff to application |
| `Erase failed` / `Program failed` | UART load aborted |
| `Load OK` | Program OK, about to reset |

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

## QSPI memory-mapped mode (XIP read fix)

`QSPI_Flash_EnableMemoryMappedMode()` uses **WeAct `02-ExtMem_Boot`**-style settings for command **`0xEB`** (not the old 6-dummy / `0x00` alt-byte config, which shifted bytes at `0x90000000`).

| Parameter | Value |
|-----------|--------|
| Instruction | `0xEB` fast read quad I/O |
| Alternate byte | **`0xEF`** (`W25Q64_MMAP_CONTINUOUS_READ_ALT`) |
| Dummy cycles | **4** (`W25Q64_DUMMY_CYCLES_READ_QUAD_IO`, WeAct SPI: 6−2) |
| Indirect read `0x0B` | Unchanged (**8** dummy cycles) |

**Verified:** CmdRd and Mmap both report the same SP/PC (e.g. `SP=24080000`, `PC=90000C5D` for Arundhati). Bootloader jump uses the mmap view.

---

## Application image requirements

| Requirement | Detail |
|-------------|--------|
| Linker FLASH | **`0x90000000`**, length ≤ 8 MB |
| Initial SP | Must match app RAM (e.g. **`0x24080000`**) |
| Reset vector | Thumb address in flash (e.g. **`0x90000C5D`**); host tool may print **`0x90000C5C`** (`& ~1` for display) |
| `SystemInit` | Set **`SCB->VTOR = 0x90000000`**; re-enable **I/D cache** after jump (bootloader disables cache before handoff) |
| Hex / program | Image programs @ QSPI offset **0**; only bytes in the hex file are written (gaps stay `0xFF` after erase) |

If jump reaches **HardFault** at **`0x90000A8C`**, that is the app’s **`HardFault_Handler`** (vector word **`0x90000A8D`** = handler + Thumb bit), not a wrong bootloader reset address.

---

## Status summary

| Item | Status |
|------|--------|
| Bootloader @ `0x08000000` (128 KB) | Done |
| `QSPI_Flash_Init` + QE on boot | Done |
| `app_load_flag_sanitize` | Done |
| UART load: **8 MB** erase + program + ACK | Done |
| Load fail: no reset, flag 1, mmap off | Done |
| Validate before mmap (`qspi_app_is_valid_at_flash`) | Done |
| Mmap XIP aligned with WeAct (`0xEF`, 4 dummies) | Done |
| Jump when flag 0 (`qspi_app_jump_to_application`) | Done |
| Host tool `saptashri_flash.py` (300 s erase timeout) | Done |
| **Application** @ `0x90000000` (separate project) | Build & debug in app project |
| Optional: peripheral deinit before jump | Not done |

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
| 1 | `SP` (repeat until ready) | Idle: `app_load_enable()` → `qspi_new_app_load()`; or boot with flag **1** |
| 2 | Wait | Mmap off → `true\r\n` or `false\r\n` |
| 3 | Wait | Erase full 8 MB (64 KB blocks) → `EC\r\n` (~1–3 min) |
| 4 | Wait | `WE\r\n` |
| 5 | **`S`** + 4-byte LE size | **`K`** / **`N`** |
| 6 | ≤256 B chunks | **`Y`** / **`N`** per chunk |
| 7 | — | **Success:** flag 0, mmap on, **reset**. **Fail:** flag 1, stay in bootloader |

No ST-LINK required for application updates after the bootloader is programmed once.

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
| `Peripherals/QSPI_Flash/` | Init, QE, erase, write, mmap on/off |
| `Peripherals/UART1/` | `uart_mcal` — send/recv on USART1 |
| `Core/Src/main.c` | Boot branches + idle **`SP`** trigger |
| `Features/qspi_app_load/` | Full UART program session |
| `Features/qspi_app_jump/` | Indirect validate + XIP jump |
| `Features/app_shared_ram/` | BKPSRAM flag + sanitize |

Internal bootloader @ `0x08000000`: flash once via CubeIDE / ST-LINK.

---

## Copyright

Copyright © 2026 Sibun. All rights reserved.
