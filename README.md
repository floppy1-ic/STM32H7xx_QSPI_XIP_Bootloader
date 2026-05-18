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
│   └── UART1/                      USART1 MCAL (PA9/PA10)
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

1. CubeMX init (GPIO, QSPI, SPI4, TIM1).
2. LCD welcome: **Saptashri Secure / XIP Bootloader**.
3. Read flag:
   - **1** → `qspi_new_app_load()` (does not return).
   - **0** → LCD **App load: OFF / Jump to app** → mmap + validate @ `0x90000000` → jump if valid, else bootloader poll (planned; LCD + poll done today).
   - Invalid → LCD warning, `app_load_disable()`.
4. **`while(1)`:** heartbeat on **PE3**; UART idle receive — if string is **`SP`**, `app_load_enable()` then `qspi_new_app_load()`; if flag **1**, call `qspi_new_app_load()`.

### `qspi_new_app_load()` (stub)

Today (`Features/qspi_app_load`):

1. LCD: **At app load (Stub)** for 5 s.
2. `app_load_disable()` → flag **0**.
3. `NVIC_SystemReset()`.

**Planned:** disable mmap → erase/write `app.bin` @ offset 0 → clear flag → mmap on → soft reset.

### Jump path when flag is 0 (planned in firmware)

1. LCD **App load: OFF / Jump to app** (done).
2. `QSPI_Flash_EnableMemoryMappedMode` → `qspi_app_is_valid(0x90000000)`.
3. **Valid** → `qspi_app_jump_to_application()` → application running.
4. **Invalid** → stay in bootloader: PE3 heartbeat, poll flag for load request.

---

## Host flash tool — `saptashri_flash.py`

UART programming for **W25Q64** at flash offset **`0x00000000`**.  
Bootloader enables **memory-mapped mode** and runs the app at **`0x90000000`** after reset.

**Hardware:** USART1 **PA9/PA10** (WeAct USB‑VCP), **115200 8N1**.

### Install

```bash
pip install -r tools/requirements.txt
```

### Protocol

| Step | Host | Bootloader |
|------|------|------------|
| 1 | `SP` + 4-byte LE **size** | Idle: `app_load_enable()`, `qspi_new_app_load()` |
| 2 | Wait | QSPI init, mmap off, sector erase |
| 3 | Wait | **ACK** `0x79` — ready for data |
| 4 | ≤256 B chunk | Write @ 0x0, **ACK** per chunk |
| 5 | — | mmap on, flag 0, **reset** |

No ST-LINK required for application updates.

**Today:** idle loop recognizes **`SP`** on UART (`Peripherals/UART1`, `strcmp` in `main.c`). Full **size + ACK + chunk program** path in `qspi_new_app_load()` is still planned.

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

## Build (STM32CubeIDE)

- Target: **STM32H750VBTx**, project **STM32H7xx_QSPI**.
- Add include paths if a new `Features/*/inc` folder is added:
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
| External flash | W25Q64 on QUADSPI BK1 (`PB2`, `PB6`, `PD11`–`PD13`, `PE2`) |
| LCD | ST7735 on SPI4, backlight TIM1_CH2N **PE10** |
| Heartbeat LED | **PE3** |

See `Peripherals/QSPI_Flash` and `Peripherals/SPI4_LCD` sources and CubeMX `.ioc` for pin details.

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
| Real QSPI program in `qspi_new_app_load` | Planned |
| Jump to app when flag 0 | Planned |
| Host UART `SP` trigger in idle loop | Done (stub load path) |
| Host UART size + ACK + chunk program | Planned |

---

## Copyright

Copyright © 2026 Sibun. All rights reserved.
