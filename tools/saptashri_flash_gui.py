#!/usr/bin/env python3
"""
Saptashri Secure XIP Bootloader — GUI host flash tool.

Thin Tkinter front-end over tools/saptashri_flash.py. The flash protocol/core
(`saptashri_flash.py`) is reused unchanged; this file only adds the GUI and
drives the core's lower-level steps so each stage can update widgets:

  * Browse for a .hex / .bin firmware image.
  * Pick + refresh COM ports, Connect / Disconnect toggle.
  * Image info: size (bytes + KB), sectors to erase (incl. margin), run address,
    initial SP / reset vector.
  * RGB status lights for Erase and Write-Enable (red idle -> blue active ->
    green done) with the erase time.
  * Progress bar + percent + elapsed time during the chunk transfer.
  * Auto-disconnect after a successful flash.
  * Clear button to reset path / settings / status / log.

Run:  python tools/saptashri_flash_gui.py
"""

from __future__ import annotations

import os
import struct
import sys
import threading
import time
from pathlib import Path

import tkinter as tk
from tkinter import filedialog, messagebox, ttk

# Reuse the protocol/core from the CLI tool without modifying it.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
try:
    import saptashri_flash as core
    import serial
except ImportError as exc:  # pragma: no cover - surfaced via dialog in main()
    _IMPORT_ERROR = exc
    core = None
    serial = None
else:
    _IMPORT_ERROR = None


# --- Palette (modern, GitHub-ish) ----------------------------------------------
BG = "#eaeef3"
SURFACE = "#ffffff"
BORDER = "#d8dee6"
TEXT = "#1f2328"
MUTED = "#656d76"
ACCENT = "#0969da"
ACCENT_HOVER = "#0860c4"
ACCENT_DISABLED = "#a6c8f0"
GHOST = "#eef1f5"
GHOST_HOVER = "#e1e7ee"

RED = "#cf222e"
BLUE = "#0969da"
GREEN = "#1a7f37"
IDLE_GREY = "#d0d7de"

# Log (dark console) colors
LOG_BG = "#0d1117"
LOG_FG = "#c9d1d9"
LOG_INFO = "#58a6ff"
LOG_DONE = "#3fb950"
LOG_ERR = "#f85149"
LOG_WARN = "#d29922"


class StatusLight(tk.Frame):
    """RGB indicator: red (idle) -> blue (active) -> green (done)."""

    STATES = {"idle": RED, "active": BLUE, "done": GREEN, "off": IDLE_GREY}

    def __init__(self, parent: tk.Misc, title: str) -> None:
        super().__init__(parent, bg=SURFACE)
        self.canvas = tk.Canvas(
            self, width=22, height=22, highlightthickness=0, bg=SURFACE
        )
        self._halo = self.canvas.create_oval(2, 2, 20, 20, fill="", outline="")
        self._dot = self.canvas.create_oval(5, 5, 17, 17, fill=RED, outline="")
        self.canvas.grid(row=0, column=0, rowspan=2, padx=(0, 8))
        tk.Label(
            self, text=title, bg=SURFACE, fg=TEXT, font=("Segoe UI", 9, "bold")
        ).grid(row=0, column=1, sticky="w")
        self._state = tk.StringVar(value="Idle")
        tk.Label(
            self, textvariable=self._state, bg=SURFACE, fg=MUTED, font=("Segoe UI", 8)
        ).grid(row=1, column=1, sticky="w")

    def set_state(self, state: str, label: str | None = None) -> None:
        color = self.STATES.get(state, IDLE_GREY)
        self.canvas.itemconfig(self._dot, fill=color)
        self.canvas.itemconfig(self._halo, fill=color if state == "active" else "")
        if label is not None:
            self._state.set(label)


class FlashGuiApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.flasher: "core.SaptashriUartFlasher | None" = None
        self.image: bytes | None = None
        self.image_path: Path | None = None
        self.port_map: dict[str, str] = {}
        self.connected = False
        self.flashing = False

        root.title("Saptashri XIP — Flash Tool")
        root.configure(bg=BG)
        root.minsize(440, 500)
        root.geometry("480x560")
        root.resizable(True, True)

        self._init_styles()
        self._build_widgets()
        self.refresh_ports()
        self._update_controls()

    # --- styling -----------------------------------------------------------------
    def _init_styles(self) -> None:
        style = ttk.Style()
        try:
            style.theme_use("clam")
        except tk.TclError:
            pass

        style.configure("TFrame", background=SURFACE)
        style.configure("TLabel", background=SURFACE, foreground=TEXT, font=("Segoe UI", 9))
        style.configure("Muted.TLabel", background=SURFACE, foreground=MUTED, font=("Segoe UI", 8))
        style.configure(
            "Value.TLabel", background=SURFACE, foreground=TEXT, font=("Consolas", 9, "bold")
        )
        style.configure(
            "Section.TLabel", background=SURFACE, foreground=ACCENT, font=("Segoe UI", 9, "bold")
        )

        style.configure(
            "Accent.TButton", background=ACCENT, foreground="#ffffff",
            borderwidth=0, padding=(10, 7), font=("Segoe UI", 9, "bold"),
        )
        style.map(
            "Accent.TButton",
            background=[("active", ACCENT_HOVER), ("disabled", ACCENT_DISABLED)],
            foreground=[("disabled", "#f0f0f0")],
        )
        style.configure(
            "Ghost.TButton", background=GHOST, foreground=TEXT,
            borderwidth=0, padding=(10, 6), font=("Segoe UI", 9),
        )
        style.map(
            "Ghost.TButton",
            background=[("active", GHOST_HOVER), ("disabled", "#f4f6f8")],
            foreground=[("disabled", "#a0a8b0")],
        )

        style.configure(
            "TCombobox", fieldbackground=SURFACE, background=GHOST, borderwidth=1,
            relief="flat", padding=3,
        )
        style.configure("TEntry", fieldbackground=SURFACE, borderwidth=1, padding=4)
        style.configure(
            "Flash.Horizontal.TProgressbar",
            troughcolor="#e6eaef", background=GREEN, borderwidth=0, thickness=14,
        )

    def _card(self, parent: tk.Misc, title: str | None = None) -> tk.Frame:
        card = tk.Frame(
            parent, bg=SURFACE, highlightbackground=BORDER, highlightthickness=1, bd=0
        )
        card.pack(fill="x", padx=12, pady=(0, 8))
        inner = tk.Frame(card, bg=SURFACE)
        inner.pack(fill="x", padx=12, pady=9)
        if title:
            ttk.Label(inner, text=title, style="Section.TLabel").pack(
                anchor="w", pady=(0, 6)
            )
        return inner

    # --- layout ------------------------------------------------------------------
    def _build_widgets(self) -> None:
        header = tk.Frame(self.root, bg=ACCENT)
        header.pack(fill="x")
        inner = tk.Frame(header, bg=ACCENT)
        inner.pack(fill="x", padx=14, pady=10)
        tk.Label(
            inner, text="Saptashri Secure XIP", bg=ACCENT, fg="#ffffff",
            font=("Segoe UI", 13, "bold"),
        ).pack(side="left")
        tk.Label(
            inner, text="UART Flash Tool", bg=ACCENT, fg="#cfe3ff",
            font=("Segoe UI", 10),
        ).pack(side="left", padx=(8, 0))
        self.clear_btn = ttk.Button(
            inner, text="Clear", style="Ghost.TButton", command=self.clear_all
        )
        self.clear_btn.pack(side="right")

        body = tk.Frame(self.root, bg=BG)
        body.pack(fill="both", expand=True, pady=(10, 4))
        self._body = body

        self._build_file_card(body)
        self._build_port_card(body)
        self._build_info_card(body)
        self._build_status_card(body)
        self._build_log_card(body)

    def _build_file_card(self, parent: tk.Misc) -> None:
        card = self._card(parent, "1 · Firmware image  (.hex / .bin)")
        row = tk.Frame(card, bg=SURFACE)
        row.pack(fill="x")
        self.path_var = tk.StringVar(value="")
        self.path_entry = ttk.Entry(row, textvariable=self.path_var, state="readonly")
        self.path_entry.pack(side="left", fill="x", expand=True)
        self.browse_btn = ttk.Button(
            row, text="Browse…", style="Ghost.TButton", command=self.browse
        )
        self.browse_btn.pack(side="left", padx=(6, 0))

    def _build_port_card(self, parent: tk.Misc) -> None:
        card = self._card(parent, "3 · Serial port")
        row = tk.Frame(card, bg=SURFACE)
        row.pack(fill="x")
        ttk.Label(row, text="Port").pack(side="left")
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(
            row, textvariable=self.port_var, state="readonly", width=28
        )
        self.port_combo.pack(side="left", padx=6)
        self.refresh_btn = ttk.Button(
            row, text="Refresh", style="Ghost.TButton", command=self.refresh_ports
        )
        self.refresh_btn.pack(side="left")

        ttk.Label(row, text="Baud").pack(side="left", padx=(12, 0))
        self.baud_var = tk.StringVar(value=str(core.DEFAULT_BAUD if core else 115200))
        self.baud_entry = ttk.Entry(row, textvariable=self.baud_var, width=8)
        self.baud_entry.pack(side="left", padx=6)

        self.connect_btn = ttk.Button(
            row, text="Connect", style="Accent.TButton", command=self.toggle_connection
        )
        self.connect_btn.pack(side="right")

    def _build_info_card(self, parent: tk.Misc) -> None:
        card = self._card(parent, "Image info")
        grid = tk.Frame(card, bg=SURFACE)
        grid.pack(fill="x")
        self.info_vars = {
            "size": tk.StringVar(value="—"),
            "sectors": tk.StringVar(value="—"),
            "run": tk.StringVar(value="—"),
            "vectors": tk.StringVar(value="—"),
        }
        rows = [("Size", "size"), ("Erase", "sectors"), ("Run @", "run"), ("Vectors", "vectors")]
        for i, (label, key) in enumerate(rows):
            ttk.Label(grid, text=label, style="Muted.TLabel").grid(
                row=i, column=0, sticky="w", pady=1, padx=(0, 14)
            )
            ttk.Label(grid, textvariable=self.info_vars[key], style="Value.TLabel").grid(
                row=i, column=1, sticky="w", pady=1
            )
        grid.columnconfigure(1, weight=1)

    def _build_status_card(self, parent: tk.Misc) -> None:
        card = self._card(parent, "Flash status")
        lights = tk.Frame(card, bg=SURFACE)
        lights.pack(fill="x")
        self.erase_light = StatusLight(lights, "6 · Erase")
        self.erase_light.pack(side="left", padx=(0, 28))
        self.we_light = StatusLight(lights, "7 · Write enable")
        self.we_light.pack(side="left")

        prog = tk.Frame(card, bg=SURFACE)
        prog.pack(fill="x", pady=(8, 0))
        self.progress = ttk.Progressbar(
            prog, mode="determinate", maximum=100.0,
            style="Flash.Horizontal.TProgressbar",
        )
        self.progress.pack(fill="x")
        self.progress_var = tk.StringVar(value="0.0%   0.0s   0 / 0 B")
        ttk.Label(prog, textvariable=self.progress_var, style="Muted.TLabel").pack(
            anchor="w", pady=(3, 0)
        )

        self.flash_btn = ttk.Button(
            card, text="Flash", style="Accent.TButton", command=self.start_flash
        )
        self.flash_btn.pack(fill="x", pady=(9, 0))

    def _build_log_card(self, parent: tk.Misc) -> None:
        card = self._card(parent, "Log")
        wrap = tk.Frame(card, bg=SURFACE)
        wrap.pack(fill="both", expand=True)
        self.log = tk.Text(
            wrap, height=5, bg=LOG_BG, fg=LOG_FG, insertbackground=LOG_FG,
            font=("Consolas", 9), relief="flat", wrap="word", padx=8, pady=6,
            borderwidth=0,
        )
        self.log.pack(side="left", fill="both", expand=True)
        sb = ttk.Scrollbar(wrap, command=self.log.yview)
        sb.pack(side="right", fill="y")
        self.log.configure(yscrollcommand=sb.set, state="disabled")
        self.log.tag_configure("info", foreground=LOG_INFO)
        self.log.tag_configure("done", foreground=LOG_DONE)
        self.log.tag_configure("err", foreground=LOG_ERR)
        self.log.tag_configure("warn", foreground=LOG_WARN)
        self.log.tag_configure("plain", foreground=LOG_FG)

    # --- helpers -----------------------------------------------------------------
    def _ui(self, fn, *args) -> None:
        """Marshal a callable onto the Tk main thread."""
        self.root.after(0, lambda: fn(*args))

    def _log_line(self, msg: str) -> None:
        tag = "plain"
        if msg.startswith("[INFO]"):
            tag = "info"
        elif msg.startswith("[DONE]"):
            tag = "done"
        elif msg.startswith("[ERR]"):
            tag = "err"
        elif msg.startswith("[WARN]"):
            tag = "warn"
        self.log.configure(state="normal")
        self.log.insert("end", msg + "\n", tag)
        self.log.see("end")
        self.log.configure(state="disabled")

    def log_line(self, msg: str) -> None:
        self._ui(self._log_line, msg)

    # --- port handling -----------------------------------------------------------
    def refresh_ports(self) -> None:
        ports = core.list_serial_ports()
        self.port_map = {}
        values = []
        for dev, desc in ports:
            text = f"{dev} — {desc}" if desc else dev
            self.port_map[text] = dev
            values.append(text)
        self.port_combo["values"] = values
        if values:
            if self.port_var.get() not in values:
                self.port_var.set(values[0])
        else:
            self.port_var.set("")
        self._log_line(f"[INFO] Found {len(values)} serial port(s)")

    def _selected_port(self) -> str | None:
        return self.port_map.get(self.port_var.get())

    # --- file handling -----------------------------------------------------------
    def browse(self) -> None:
        path = filedialog.askopenfilename(
            title="Select firmware (.hex / .bin)",
            filetypes=[("Firmware images", "*.hex *.bin"), ("HEX", "*.hex"),
                       ("Binary", "*.bin")],
        )
        if not path:
            return
        self.load_image_info(Path(path))

    def load_image_info(self, path: Path) -> None:
        if path.suffix.lower() not in (".hex", ".bin", ".ihex"):
            messagebox.showerror("Invalid file", "Please choose a .hex or .bin file.")
            return
        try:
            image = core.load_image(path)
        except core.FlashToolError as exc:
            messagebox.showerror("Image error", str(exc))
            return

        self.image = image
        self.image_path = path
        self.path_var.set(str(path))

        size = len(image)
        self.info_vars["size"].set(f"{size} bytes  ({size / 1024:.1f} KB)")

        n_sectors = core.sectors_to_erase(size)
        erase_bytes = n_sectors * core.W25Q64_SECTOR_SIZE
        self.info_vars["sectors"].set(
            f"{n_sectors} x 4 KB sectors  ({erase_bytes} B, +1 margin)"
        )

        self.info_vars["run"].set(f"0x{core.QSPI_XIP_BASE:08X}")
        if size >= 8:
            sp = int.from_bytes(image[0:4], "little")
            reset = int.from_bytes(image[4:8], "little") & ~1
            self.info_vars["vectors"].set(f"SP=0x{sp:08X}  reset=0x{reset:08X}")
        else:
            self.info_vars["vectors"].set("image too short")

        self._log_line(f"[INFO] Loaded {path.name}")
        self._log_line(f"[INFO] Run @ 0x{core.QSPI_XIP_BASE:08X}")
        self._log_line(f"[INFO] Image: {size} bytes @ QSPI 0x{core.QSPI_FLASH_BASE:08X}")
        self._update_controls()

    # --- clear -------------------------------------------------------------------
    def clear_all(self) -> None:
        if self.flashing:
            return
        if self.connected:
            self.disconnect()
        self.image = None
        self.image_path = None
        self.path_var.set("")
        for var in self.info_vars.values():
            var.set("—")
        self.baud_var.set(str(core.DEFAULT_BAUD if core else 115200))
        self._reset_status_lights()
        self.log.configure(state="normal")
        self.log.delete("1.0", "end")
        self.log.configure(state="disabled")
        self._log_line("[INFO] Cleared")
        self._update_controls()

    # --- connection --------------------------------------------------------------
    def toggle_connection(self) -> None:
        if self.connected:
            self.disconnect()
        else:
            self.connect()

    def connect(self) -> None:
        port = self._selected_port()
        if not port:
            messagebox.showwarning("No port", "Select a COM port (use Refresh).")
            return
        try:
            baud = int(self.baud_var.get())
        except ValueError:
            messagebox.showwarning("Baud", "Baud must be a number.")
            return
        try:
            self.flasher = core.SaptashriUartFlasher(port, baud=baud)
        except (serial.SerialException, OSError) as exc:
            messagebox.showerror("Connect failed", str(exc))
            return
        self.connected = True
        self.connect_btn.configure(text="Disconnect")
        self._log_line(f"[DONE] Connected to {port} @ {baud}")
        self._reset_status_lights()
        self._update_controls()

    def disconnect(self, *, auto: bool = False) -> None:
        if self.flasher is not None:
            try:
                self.flasher.close()
            except Exception:
                pass
            self.flasher = None
        self.connected = False
        self.connect_btn.configure(text="Connect")
        self._log_line("[INFO] Auto-disconnected" if auto else "[INFO] Disconnected")
        self._update_controls()

    # --- control state -----------------------------------------------------------
    def _update_controls(self) -> None:
        busy = self.flashing
        can_flash = self.connected and self.image is not None and not busy

        self.browse_btn.configure(state="disabled" if busy else "normal")
        self.clear_btn.configure(state="disabled" if busy else "normal")
        self.refresh_btn.configure(
            state="disabled" if (self.connected or busy) else "normal"
        )
        self.port_combo.configure(
            state="disabled" if (self.connected or busy) else "readonly"
        )
        self.baud_entry.configure(
            state="disabled" if (self.connected or busy) else "normal"
        )
        self.connect_btn.configure(state="disabled" if busy else "normal")
        self.flash_btn.configure(state="normal" if can_flash else "disabled")

    def _reset_status_lights(self) -> None:
        self.erase_light.set_state("idle", "Idle")
        self.we_light.set_state("idle", "Idle")
        self.progress["value"] = 0.0
        self.progress_var.set("0.0%   0.0s   0 / 0 B")

    # --- flashing ----------------------------------------------------------------
    def start_flash(self) -> None:
        if self.image is None:
            messagebox.showwarning("No firmware", "Add a .hex or .bin file first.")
            return
        if not self.connected or self.flasher is None:
            messagebox.showwarning("Not connected", "Connect to a COM port first.")
            return

        self.flashing = True
        self._reset_status_lights()
        self._update_controls()
        self._log_line("[INFO] Flashing…")

        worker = threading.Thread(target=self._flash_worker, daemon=True)
        worker.start()

    def _flash_worker(self) -> None:
        assert self.flasher is not None and self.image is not None
        flasher = self.flasher
        image = self.image
        total = len(image)
        try:
            # Step 1-2: trigger + mmap off (P until 'true').
            flasher._send_sp_until_ready(progress=False)
            self.log_line("[DONE] Bootloader ready (true)")

            # Step 3: size handshake (S + LE size -> K).
            payload = core.SYNC_SIZE + struct.pack("<I", total)
            flasher.ser.write(payload)
            flasher.ser.flush()
            flasher._wait_for_byte_ack(
                core.ACK_SIZE_OK, "size", timeout=core.SIZE_ACK_TIMEOUT_S
            )
            self.log_line("[DONE] Size accepted (K)")

            # Step 4: erase (RGB: blue -> green).
            self._ui(self.erase_light.set_state, "active", "Erasing…")
            self.log_line("[INFO] Waiting for EC (erase complete)…")
            t_erase = time.monotonic()
            flasher._wait_for_token(
                core.RESP_ERASE_COMPLETE,
                core.ERASE_COMPLETE_TIMEOUT_S,
                progress=False,
                label="EC",
            )
            erase_s = time.monotonic() - t_erase
            self._ui(self.erase_light.set_state, "done", f"Done · {erase_s:.1f}s")
            self.log_line(f"[DONE] Erase complete in {erase_s:.1f}s")

            # Step 5: write enable (RGB: blue -> green).
            self._ui(self.we_light.set_state, "active", "Enabling…")
            self.log_line("[INFO] Waiting for WE (write enable)…")
            flasher._wait_for_token(
                core.RESP_WRITE_ENABLE,
                core.WRITE_ENABLE_TIMEOUT_S,
                progress=False,
                label="WE",
            )
            self._ui(self.we_light.set_state, "done", "Done")
            self.log_line("[DONE] WE (write enable) received")

            flasher._rx_pending.clear()
            time.sleep(core.POST_WE_DELAY_S)

            # Step 6: stream chunks with per-chunk ACK + progress bar.
            self.log_line(f"[INFO] Flashing in up to {core.CHUNK_SIZE}-byte chunks…")
            sent = 0
            t_flash = time.monotonic()
            while sent < total:
                chunk = image[sent : sent + core.CHUNK_SIZE]
                flasher.ser.write(chunk)
                flasher.ser.flush()
                flasher._read_ack(f"chunk @ {sent}")
                sent += len(chunk)
                self._ui(self._update_progress, sent, total, time.monotonic() - t_flash)

            flash_s = time.monotonic() - t_flash
            self.log_line(
                f"[DONE] Flash complete in {flash_s:.1f}s — "
                f"MCU resets → app @ 0x{core.QSPI_XIP_BASE:08X}"
            )
            self._ui(self._on_flash_success)
        except Exception as exc:  # FlashToolError, SerialException, etc.
            self.log_line(f"[ERR] {exc}")
            self._ui(self._on_flash_failure)

    def _update_progress(self, sent: int, total: int, elapsed: float) -> None:
        pct = 100.0 * sent / total if total else 0.0
        self.progress["value"] = pct
        self.progress_var.set(f"{pct:5.1f}%   {elapsed:5.1f}s   {sent} / {total} B")

    def _on_flash_success(self) -> None:
        self.flashing = False
        messagebox.showinfo(
            "Flash complete",
            f"Programmed {len(self.image)} bytes.\n"
            f"MCU resets → app @ 0x{core.QSPI_XIP_BASE:08X}.\nAuto-disconnecting.",
        )
        self.disconnect(auto=True)

    def _on_flash_failure(self) -> None:
        self.flashing = False
        self.erase_light.set_state("idle", "Idle")
        self.we_light.set_state("idle", "Idle")
        self._update_controls()
        messagebox.showerror("Flash failed", "See log for details. Still connected.")


def main() -> int:
    root = tk.Tk()
    if _IMPORT_ERROR is not None:
        messagebox.showerror(
            "Missing dependency",
            f"Could not import core / pyserial:\n{_IMPORT_ERROR}\n\n"
            "Install with: pip install -r tools/requirements.txt",
        )
        root.destroy()
        return 1
    FlashGuiApp(root)
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
