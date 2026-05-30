#!/usr/bin/env python3
"""
Saptashri Secure XIP Bootloader — UART host flash tool.

Protocol (must match qspi_app_load.c):
  1. Host: P (repeat)            MCU: true\\r\\n or false\\r\\n
  2. Host: S + 4-byte LE size      MCU: K (ok) or N
  3. MCU: erase ceil(size/4K)+1 sectors  Host: wait EC\\r\\n
  4. MCU: WE\\r\\n                 Host: wait WE
  5. Host: chunks (<=256 B)       MCU: Y or N per chunk
  6. Success: MCU mmap on + reset. Failure: MCU stays in bootloader (no reset); host sees N or EC timeout

Examples:
  python tools/saptashri_flash.py
  python tools/saptashri_flash.py -p COM3 app.bin
  python tools/saptashri_flash.py -p COM3 -y app.hex
  python tools/saptashri_flash.py --info app.hex
"""

from __future__ import annotations

import argparse
import os
import struct
import sys
import time
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Install: pip install -r tools/requirements.txt", file=sys.stderr)
    raise

QSPI_FLASH_BASE = 0x0000_0000
QSPI_FLASH_SIZE = 8 * 1024 * 1024
QSPI_XIP_BASE = 0x9000_0000

LOAD_TRIGGER = b"P"
RESP_TRUE = b"true"
RESP_FALSE = b"false"
RESP_ERASE_COMPLETE = b"EC"
RESP_WRITE_ENABLE = b"WE"
ACK = ord("Y")
NACK = ord("N")
CHUNK_SIZE = 256
W25Q64_SECTOR_SIZE = 4 * 1024
FLASH_ERASE_EXTRA_SECTORS = 1

DEFAULT_BAUD = 115200
DEFAULT_TIMEOUT = 5.0
ACK_TIMEOUT = 120.0
ERASE_COMPLETE_TIMEOUT_S = 120.0
WRITE_ENABLE_TIMEOUT_S = 30.0
SIZE_ACK_TIMEOUT_S = 30.0
POST_WE_DELAY_S = 0.08
POST_SIZE_DELAY_S = 0.05
SYNC_SIZE = b"S"
ACK_SIZE_OK = ord("K")

SP_TRIGGER_INTERVAL_S = 0.05
SP_TRIGGER_DURATION_S = 12.0
SP_RESPONSE_POLL_S = 0.1
POST_READY_DELAY_S = 0.5
TOKEN_POLL_S = 0.1

PROGRESS_BAR_WIDTH = 50


def sectors_to_erase(image_size: int) -> int:
    """4 KB sectors covering image plus one extra margin sector (matches firmware)."""
    sectors_for_image = (image_size + W25Q64_SECTOR_SIZE - 1) // W25Q64_SECTOR_SIZE
    return sectors_for_image + FLASH_ERASE_EXTRA_SECTORS


# --- Terminal UI (ANSI); flash protocol unchanged ---------------------------------
class _Ansi:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    DIM = "\033[2m"
    RED = "\033[31m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    BLUE = "\033[34m"
    MAGENTA = "\033[35m"
    CYAN = "\033[36m"
    WHITE = "\033[97m"
    BG_BLUE = "\033[44m"


def _color_enabled() -> bool:
    if os.environ.get("NO_COLOR"):
        return False
    return hasattr(sys.stdout, "isatty") and sys.stdout.isatty()


def _paint(text: str, *codes: str) -> str:
    if not _color_enabled() or not codes:
        return text
    return "".join(codes) + text + _Ansi.RESET


def _addr(hex_val: int) -> str:
    return _paint(f"0x{hex_val:08X}", _Ansi.CYAN, _Ansi.BOLD)


def _tag(label: str, color: str) -> str:
    return _paint(f"[{label}]", color, _Ansi.BOLD)


def _println(msg: str = "", *, file=None) -> None:
    print(msg, file=file)


def _print_info(msg: str) -> None:
    _println(f"{_tag('INFO', _Ansi.BLUE)} {msg}")


def _print_done(msg: str) -> None:
    _println(f"{_tag('DONE', _Ansi.GREEN)} {msg}")


def _print_warn(msg: str) -> None:
    _println(f"{_tag('WARN', _Ansi.YELLOW)} {msg}")


def _print_err(msg: str, *, file=None) -> None:
    _println(f"{_tag('ERR', _Ansi.RED)} {msg}", file=file or sys.stderr)


def _format_vectors_summary(vector_line: str) -> str:
    if vector_line.startswith("OK"):
        return _paint(vector_line, _Ansi.GREEN)
    if "not in XIP" in vector_line or "too short" in vector_line:
        return _paint(vector_line, _Ansi.RED)
    return _paint(vector_line, _Ansi.YELLOW)


def _rewrite_prompt_as_done(message: str) -> None:
    """Replace the current input line with a [DONE] line (same row)."""
    sys.stdout.write(f"\r\033[2K{message}\n")
    sys.stdout.flush()


def print_image_summary(path: Path, image: bytes, vector_line: str) -> None:
    _print_info(f"File: {_paint(path.name, _Ansi.BOLD, _Ansi.WHITE)}")
    _print_info(
        f"Program @ {_addr(QSPI_FLASH_BASE)}  "
        f"{_paint(f'({len(image)} bytes)', _Ansi.DIM)}"
    )
    _print_info(
        f"Run @ {_addr(QSPI_XIP_BASE)}  "
        f"{_paint('(bootloader after reset)', _Ansi.DIM)}"
    )
    _print_info(f"Vectors: {_format_vectors_summary(vector_line)}")


def print_serial_ports(ports: list[tuple[str, str]]) -> None:
    _print_info(_paint("Serial ports", _Ansi.BOLD, _Ansi.MAGENTA))
    _println(_paint("─" * 48, _Ansi.DIM))
    for i, (dev, desc) in enumerate(ports):
        idx = _paint(f"[{i}]", _Ansi.CYAN, _Ansi.BOLD)
        dev_s = _paint(dev, _Ansi.GREEN, _Ansi.BOLD)
        if desc:
            _print_info(f"{idx} {dev_s} {_paint('—', _Ansi.DIM)} {desc}")
        else:
            _print_info(f"{idx} {dev_s}")


def prompt_firmware_path() -> str:
    prompt = (
        f"{_tag('INFO', _Ansi.BLUE)} Firmware "
        f"{_paint('(.bin/.hex)', _Ansi.DIM)}: "
    )
    path = input(prompt).strip().strip('"')
    _rewrite_prompt_as_done(
        f"{_tag('DONE', _Ansi.GREEN)} {_paint(path, _Ansi.WHITE, _Ansi.BOLD)}"
    )
    return path


def print_flash_ready(port: str, baud: int) -> None:
    _print_done(
        f"Flash via {_paint(port, _Ansi.GREEN, _Ansi.BOLD)} "
        f"set to {_paint(str(baud), _Ansi.CYAN)}"
    )


def prompt_flash_confirm(port: str, baud: int) -> bool:
    hint = _paint("[Y/n]", _Ansi.GREEN)
    prompt = (
        f"{_tag('INFO', _Ansi.BLUE)} Flash via "
        f"{_paint(port, _Ansi.GREEN, _Ansi.BOLD)} @ "
        f"{_paint(str(baud), _Ansi.CYAN)} {hint}: "
    )
    ans = input(prompt).strip().lower()
    if ans in ("n", "no"):
        sys.stdout.write("\r\033[2K")
        sys.stdout.flush()
        return False
    if ans not in ("", "y", "yes"):
        _print_warn("Unknown input — treating as yes.")
    _rewrite_prompt_as_done(
        f"{_tag('DONE', _Ansi.GREEN)} Flash via "
        f"{_paint(port, _Ansi.GREEN, _Ansi.BOLD)} "
        f"set to {_paint(str(baud), _Ansi.CYAN)}"
    )
    return True


def _render_progress_bar(ratio: float, width: int = PROGRESS_BAR_WIDTH) -> str:
    ratio = max(0.0, min(1.0, ratio))
    filled = int(width * ratio)
    if ratio >= 1.0:
        filled = width
    return _paint("#" * filled + " " * (width - filled), _Ansi.GREEN)


def _write_progress_line(sent: int, total: int, t0: float) -> None:
    """Single-line progress (50 # max); clear EOL so Windows does not wrap-spam."""
    elapsed = time.monotonic() - t0
    pct = 100.0 * sent / total if total else 0.0
    done = sent >= total and total > 0
    bar = _render_progress_bar(sent / total if total else 0.0)
    if done:
        pct_s = _paint(f"{pct:5.1f}%", _Ansi.GREEN, _Ansi.BOLD)
        bytes_s = _paint(f"{sent}/{total} B", _Ansi.GREEN, _Ansi.BOLD)
    else:
        pct_s = _paint(f"{pct:5.1f}%", _Ansi.CYAN, _Ansi.BOLD)
        bytes_s = f"{sent}/{total} B"
    stats = f"{pct_s}  {_paint(f'{elapsed:5.1f}s', _Ansi.DIM)}  {bytes_s}"
    sys.stdout.write(f"\r\033[2K[{bar}]  {stats}")
    sys.stdout.flush()


class FlashToolError(Exception):
    pass


def parse_intel_hex(path: Path) -> dict[int, int]:
    upper_linear = 0
    upper_segment = 0
    memory: dict[int, int] = {}

    for raw_line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw_line.strip()
        if not line or not line.startswith(":") or len(line) < 11:
            continue
        try:
            count = int(line[1:3], 16)
            offset = int(line[3:7], 16)
            rec_type = int(line[7:9], 16)
            data = bytes.fromhex(line[9 : 9 + count * 2])
        except ValueError as exc:
            raise FlashToolError(f"Invalid HEX: {line!r}") from exc

        if rec_type == 0x00:
            base = upper_linear + upper_segment + offset
            for index, byte in enumerate(data):
                memory[base + index] = byte
        elif rec_type == 0x01:
            break
        elif rec_type == 0x02 and len(data) == 2:
            upper_segment = ((data[0] << 8) | data[1]) << 4
            upper_linear = 0
        elif rec_type == 0x04 and len(data) == 2:
            upper_linear = ((data[0] << 8) | data[1]) << 16
            upper_segment = 0

    if not memory:
        raise FlashToolError("HEX file has no data")
    return memory


def load_image(path: Path) -> bytes:
    suffix = path.suffix.lower()
    if suffix == ".bin":
        data = path.read_bytes()
        if not data:
            raise FlashToolError("Empty .bin")
        return data

    if suffix in (".hex", ".ihex"):
        memory = parse_intel_hex(path)
        addrs = sorted(memory)

        def to_offset(addr: int) -> int:
            if addr == QSPI_FLASH_BASE:
                return QSPI_FLASH_BASE
            if addr == QSPI_XIP_BASE:
                return QSPI_FLASH_BASE
            if QSPI_XIP_BASE <= addr < QSPI_XIP_BASE + QSPI_FLASH_SIZE:
                return addr - QSPI_XIP_BASE
            raise FlashToolError(
                f"HEX address 0x{addr:08X} must be 0x0 or 0x{QSPI_XIP_BASE:08X}"
            )

        min_off = to_offset(addrs[0])
        max_addr = max(addrs)
        max_off = to_offset(max_addr)
        image = bytearray(b"\xFF" * (max_off - min_off + 1))
        for addr, byte in memory.items():
            image[to_offset(addr) - min_off] = byte
        if min_off != QSPI_FLASH_BASE:
            raise FlashToolError("Image must start at flash offset 0 or XIP base")
        return bytes(image)

    raise FlashToolError("Use .bin or .hex")


def inspect_vectors(data: bytes) -> str:
    if len(data) < 8:
        return "too short"
    sp = int.from_bytes(data[0:4], "little")
    reset = int.from_bytes(data[4:8], "little") & ~1
    if not (0x2000_0000 <= sp <= 0x2408_FFFF):
        return f"SP 0x{sp:08X} unusual"
    if not (QSPI_XIP_BASE <= reset < QSPI_XIP_BASE + QSPI_FLASH_SIZE):
        return f"reset 0x{reset:08X} not in XIP"
    return f"OK  SP=0x{sp:08X}  reset=0x{reset:08X}"


class SaptashriUartFlasher:
    def __init__(self, port: str, baud: int = DEFAULT_BAUD, timeout: float = DEFAULT_TIMEOUT) -> None:
        self.ser = serial.Serial(
            port=port,
            baudrate=baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=timeout,
        )
        self._rx_pending = bytearray()
        self.ser.reset_input_buffer()

    def close(self) -> None:
        if self.ser.is_open:
            self.ser.close()

    def _wait_for_byte_ack(
        self, expected: int, context: str, timeout: float = ACK_TIMEOUT
    ) -> None:
        """Poll until expected ACK byte (skip \\r\\n; drain stray ASCII)."""
        deadline = time.monotonic() + timeout

        while time.monotonic() < deadline:
            self._poll_serial_into_pending()

            while self._rx_pending and self._rx_pending[0] in (0x0D, 0x0A):
                del self._rx_pending[0]

            if self._rx_pending:
                value = self._rx_pending.pop(0)
            else:
                time.sleep(TOKEN_POLL_S)
                continue

            if value == expected:
                return
            if value == NACK:
                raise FlashToolError(f"Bootloader NACK ({context})")

        tail = self._rx_pending.decode("utf-8", errors="replace").strip()
        hint = f" Last RX: {tail!r}" if tail else ""
        raise FlashToolError(
            f"Timeout waiting for {chr(expected)!r} ({context}){hint}. "
            "Rebuild & flash bootloader (ST-Link), then retry."
        )

    def _read_ack(self, context: str, timeout: float = ACK_TIMEOUT) -> None:
        self._wait_for_byte_ack(ACK, context, timeout)

    def _send_sp_until_ready(self, *, progress: bool = True) -> None:
        """Send SP until RX contains 'true' (ready) or 'false' (mmap disable failed)."""
        self.ser.reset_input_buffer()
        rx = bytearray()
        deadline = time.monotonic() + SP_TRIGGER_DURATION_S
        attempt = 0

        if progress:
            _print_info(
                f"Sending P every {int(SP_TRIGGER_INTERVAL_S * 1000)} ms "
                f"(up to {SP_TRIGGER_DURATION_S:.0f} s) until true/false..."
            )

        while time.monotonic() < deadline:
            attempt += 1
            self.ser.write(LOAD_TRIGGER)
            self.ser.flush()

            old = self.ser.timeout
            self.ser.timeout = SP_RESPONSE_POLL_S
            try:
                chunk = self.ser.read(512)
            finally:
                self.ser.timeout = old

            if chunk:
                rx.extend(chunk)
                if RESP_FALSE in rx:
                    raise FlashToolError(
                        "Bootloader returned false (memory map disable failed)"
                    )
                if RESP_TRUE in rx:
                    if progress:
                        _print_done(
                            f"Bootloader ready "
                            f"{_paint('(true)', _Ansi.GREEN)} after {attempt} P(s)."
                        )
                    time.sleep(POST_READY_DELAY_S)
                    self._rx_pending.clear()
                    self.ser.reset_input_buffer()
                    return

            time.sleep(SP_TRIGGER_INTERVAL_S)

        tail = rx.decode("utf-8", errors="replace").strip()
        hint = f" Last RX: {tail!r}" if tail else ""
        raise FlashToolError(
            f"No true/false within {SP_TRIGGER_DURATION_S:.0f} s "
            f"({attempt} SP sends). Is the bootloader idle on UART?{hint}"
        )

    def _poll_serial_into_pending(self) -> None:
        old = self.ser.timeout
        self.ser.timeout = TOKEN_POLL_S
        try:
            chunk = self.ser.read(512)
        finally:
            self.ser.timeout = old
        if chunk:
            self._rx_pending.extend(chunk)

    def _wait_for_token(
        self,
        token: bytes,
        deadline_s: float,
        *,
        progress: bool,
        label: str,
    ) -> None:
        """Accumulate RX until token appears; keep bytes after token for the next wait."""
        deadline = time.monotonic() + deadline_s

        if progress:
            _print_info(f"Waiting for {label}...")

        while time.monotonic() < deadline:
            self._poll_serial_into_pending()

            if RESP_FALSE in self._rx_pending:
                raise FlashToolError(f"Bootloader false while waiting for {label}")
            if bytes([NACK]) in self._rx_pending:
                raise FlashToolError(f"Bootloader NACK while waiting for {label}")

            idx = self._rx_pending.find(token)
            if idx >= 0:
                del self._rx_pending[: idx + len(token)]
                if progress:
                    _print_done(f"{label} received")
                return

            time.sleep(TOKEN_POLL_S)

        tail = self._rx_pending.decode("utf-8", errors="replace").strip()
        hint = f" Last RX: {tail!r}" if tail else ""
        raise FlashToolError(f"Timeout waiting for {label}{hint}")

    def program(self, image: bytes, *, progress: bool = True) -> None:
        if len(image) > QSPI_FLASH_SIZE:
            raise FlashToolError(f"Image {len(image)} B > W25Q64 size")
        if len(image) == 0:
            raise FlashToolError("Empty image")

        n_sectors = sectors_to_erase(len(image))
        erase_bytes = n_sectors * W25Q64_SECTOR_SIZE
        if erase_bytes > QSPI_FLASH_SIZE:
            raise FlashToolError(
                f"Erase window {erase_bytes} B exceeds W25Q64 ({QSPI_FLASH_SIZE} B)"
            )

        if progress:
            _print_info(
                f"Image: {len(image)} bytes @ QSPI {_addr(QSPI_FLASH_BASE)}"
            )
            _print_info(
                f"Erase: {n_sectors} x 4 KB sectors "
                f"({_paint(f'{erase_bytes} B', _Ansi.DIM)}, +1 margin)"
            )

        self._send_sp_until_ready(progress=progress)

        size_payload = SYNC_SIZE + struct.pack("<I", len(image))
        self.ser.write(size_payload)
        self.ser.flush()
        if progress:
            _print_info(
                f"Sent size sync + {len(image)} bytes "
                f"({_paint(size_payload.hex(), _Ansi.DIM)})"
            )

        self._wait_for_byte_ack(ACK_SIZE_OK, "size", timeout=SIZE_ACK_TIMEOUT_S)
        if progress:
            _print_done("Size accepted (K)")

        if progress:
            _print_info(f"Erasing {n_sectors} sector(s)...")

        t_erase = time.monotonic()
        self._wait_for_token(
            RESP_ERASE_COMPLETE,
            ERASE_COMPLETE_TIMEOUT_S,
            progress=progress,
            label="EC (erase complete)",
        )
        if progress:
            erase_elapsed = time.monotonic() - t_erase
            _print_done(
                f"Erase complete in {_paint(f'{erase_elapsed:.1f}s', _Ansi.CYAN)}"
            )

        self._wait_for_token(
            RESP_WRITE_ENABLE,
            WRITE_ENABLE_TIMEOUT_S,
            progress=progress,
            label="WE (write enable)",
        )

        self._rx_pending.clear()
        time.sleep(POST_WE_DELAY_S)

        if progress:
            _print_info(f"Flashing in up to {CHUNK_SIZE}-byte chunks...")

        sent = 0
        t_flash = time.monotonic()
        total = len(image)
        while sent < total:
            chunk = image[sent : sent + CHUNK_SIZE]
            self.ser.write(chunk)
            self.ser.flush()
            self._read_ack(f"chunk @ {sent}")
            sent += len(chunk)
            if progress:
                _write_progress_line(sent, total, t_flash)
        if progress:
            flash_elapsed = time.monotonic() - t_flash
            print()
            _print_done(
                f"Flash complete in {_paint(f'{flash_elapsed:.1f}s', _Ansi.CYAN)} — "
                f"MCU resets → app @ {_addr(QSPI_XIP_BASE)}"
            )


def list_serial_ports() -> list[tuple[str, str]]:
    return [(p.device, p.description or "") for p in list_ports.comports()]


def prompt_port() -> str:
    ports = list_serial_ports()
    if not ports:
        _print_err("No serial ports found.")
        raise SystemExit(1)
    print_serial_ports(ports)
    while True:
        prompt = (
            f"{_tag('INFO', _Ansi.BLUE)} Select "
            f"{_paint(f'[0-{len(ports) - 1}]', _Ansi.CYAN)}: "
        )
        c = input(prompt).strip()
        if c.isdigit() and 0 <= int(c) < len(ports):
            chosen = ports[int(c)][0]
            _rewrite_prompt_as_done(
                f"{_tag('DONE', _Ansi.GREEN)} Using "
                f"{_paint(chosen, _Ansi.GREEN, _Ansi.BOLD)}"
            )
            return chosen


def main(argv: list[str] | None = None) -> int:
    argv = list(argv if argv is not None else sys.argv[1:])

    parser = argparse.ArgumentParser(description="UART flash to W25Q64 @ 0x0 (Saptashri bootloader)")
    parser.add_argument("-p", "--port", help="COM port (interactive if omitted)")
    parser.add_argument("-b", "--baud", type=int, default=DEFAULT_BAUD)
    parser.add_argument("-y", "--yes", action="store_true", help="Skip confirmation")
    parser.add_argument("firmware", nargs="?", help=".bin or .hex")
    parser.add_argument("--info", action="store_true", help="Show image info only (no UART)")
    args = parser.parse_args(argv)

    if not args.firmware:
        if args.info:
            parser.error("firmware path required for --info")
        args.firmware = prompt_firmware_path()
    else:
        _print_done(_paint(str(args.firmware), _Ansi.WHITE, _Ansi.BOLD))

    path = Path(args.firmware)
    if not path.is_file():
        _print_err(f"Not found: {path}")
        return 1

    try:
        image = load_image(path)
    except FlashToolError as exc:
        _print_err(str(exc))
        return 1

    vector_line = inspect_vectors(image)
    print_image_summary(path, image, vector_line)

    if args.info:
        return 0

    port = args.port or prompt_port()
    if args.yes:
        print_flash_ready(port, args.baud)
    elif not prompt_flash_confirm(port, args.baud):
        _print_info("Flash cancelled.")
        return 0

    try:
        flasher = SaptashriUartFlasher(port, baud=args.baud)
        flasher.program(image)
        flasher.close()
    except (FlashToolError, serial.SerialException) as exc:
        _print_err(str(exc))
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
