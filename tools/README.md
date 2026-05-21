# Host flash tool — `saptashri_flash.py`

UART programming for **W25Q64** at flash offset **`0x00000000`**.  
Bootloader enables **memory-mapped mode** and runs the app at **`0x90000000`** after reset.

**Hardware:** USART1 **PA9/PA10** (WeAct USB‑VCP), **115200 8N1**.

## Install

```bash
pip install -r tools/requirements.txt
```

## Protocol

| Step | Host | Bootloader |
|------|------|------------|
| 1 | `SP` every **50 ms** for up to **5 s** | Idle: detect `SP`, enter load path |
| 2 | Wait | **`true`** = mmap off OK (MCU resets); **`false`** = failed |
| 3 | ≤256 B chunk | Write @ 0x0, **ACK** per chunk |
| 4 | — | mmap on, flag 0, **reset** (firmware TBD) |

No ST-LINK required for application updates.

## Usage

```bash
python tools/saptashri_flash.py
python tools/saptashri_flash.py -p COM5 app.bin
python tools/saptashri_flash.py -p COM5 --info app.hex
```

`.hex` linked at **`0x90000000`** is converted to offset **0** automatically.

## Firmware

- `Features/uart_boot/` — USART1 + `SP` trigger parser  
- `Features/qspi_app_load/` — erase, UART RX, `QSPI_Flash_Write`  
- `Core/Src/main.c` — idle poll calls load on `SP`+size  

Internal bootloader @ `0x08000000`: flash once via CubeIDE / ST-LINK.
