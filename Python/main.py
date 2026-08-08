"""
main.py - Spotify Remote desktop application (Windows).

Bridges the ESP32 Receiver's USB serial link to Windows multimedia keys
so the wireless remote controls Spotify / YouTube Music / VLC / WMP.

Usage:
    python main.py                  normal start (tray + settings window)
    pythonw main.py --hidden        hidden start (tray only, no console)
    python main.py --port COM3      force a COM port

The app auto-detects the receiver, reconnects automatically, shows a
system tray icon with notifications, and can register itself to start
with Windows (see tray menu -> Start with Windows).
"""

import argparse
import logging
import os
import queue
import sys
import threading
import tkinter as tk
from tkinter import ttk

import psutil

from logger import setup_logging
from settings import (Settings, get_start_with_windows, set_start_with_windows)
from serial_manager import SerialManager
from media_controller import MediaController
from tray import TrayManager

log = logging.getLogger("spotify_remote")

# Bare command names sent by the ESP8266 receiver firmware (the legacy ESP32
# receiver sent the same values prefixed with "CMD:").
BARE_COMMANDS = frozenset({"NEXT", "PREVIOUS", "PLAY", "VOLUP", "VOLDOWN", "MUTE"})


def _is_another_instance(script_path: str) -> bool:
    """True when another process is already running this script."""
    me = os.path.normcase(os.path.abspath(script_path))
    try:
        for p in psutil.process_iter(["pid", "cmdline"]):
            try:
                if p.info["pid"] == os.getpid():
                    continue
                cmd = p.info["cmdline"] or []
                for arg in cmd:
                    if not arg:
                        continue
                    if os.path.normcase(os.path.abspath(arg)) == me:
                        return True
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue
    except Exception:
        pass
    return False


class App:
    """Wires serial -> media keys -> tray, and hosts the settings window."""

    def __init__(self, settings: Settings, hidden: bool):
        self.settings = settings
        self.hidden = hidden
        self.media = MediaController()
        self.link_up = False
        self.last_quality = None
        self.last_battery = None
        self.last_voltage = None
        self.quit_event = threading.Event()

        # All tkinter work runs on a dedicated UI thread via a task queue,
        # which keeps tray/serial threads free of Tcl interpreter issues.
        self._ui_queue = queue.Queue()
        self._ui_thread = threading.Thread(target=self._ui_worker,
                                           name="ui", daemon=True)
        self._ui_thread.start()

        self.tray = TrayManager(
            on_quit=self._on_quit,
            on_reconnect=self._on_reconnect,
            on_open_settings=self._open_settings_window,
            on_toggle_notifications=self._on_toggle_notifications,
            on_toggle_startup=self._on_toggle_startup,
            status_provider=self._status_text,
            notifications_provider=lambda: bool(self.settings.get("notifications", True)),
            startup_provider=get_start_with_windows,
        )
        self.serial = SerialManager(settings, self._on_message, self._on_link)

    # ------------------------------------------------------------- lifecycle

    def start(self):
        threading.Thread(target=self.tray.run, name="tray", daemon=True).start()
        self.serial.start()
        threading.Thread(target=self._ping_loop, name="ping", daemon=True).start()
        log.info("Application started (hidden=%s)", self.hidden)

    def _ping_loop(self):
        """Periodic keep-alive probe; also exercises the write path."""
        while not self.quit_event.is_set():
            self.quit_event.wait(30)
            if self.quit_event.is_set():
                break
            self.serial.send("PING")

    # ---------------------------------------------------------------- status

    def _status_text(self) -> str:
        parts = ["Spotify Remote", "Connected" if self.link_up else "Disconnected"]
        if self.link_up:
            if self.last_battery is not None:
                parts.append(f"Batt {self.last_battery}%")
            if self.last_quality is not None:
                parts.append(f"Link quality {self.last_quality}%")
        return " | ".join(parts)

    # -------------------------------------------------------------- messages

    def _on_link(self, up: bool):
        if up == self.link_up:
            return
        self.link_up = up
        if up:
            log.info("Receiver connected (USB)")
            if self.settings.get("notifications", True):
                self.tray.notify("Receiver Connected", "The Spotify Remote is ready.")
        else:
            log.warning("Receiver disconnected (USB)")
            if self.settings.get("notifications", True):
                self.tray.notify("Receiver Disconnected", "Reconnecting automatically...")
        self.tray.set_status(self._status_text())

    def _on_message(self, line: str):
        """Parse one serial line from the receiver firmware."""
        kind, _, value = line.partition(":")
        kind = kind.strip()
        value = value.strip()

        if kind in BARE_COMMANDS:
            self.media.execute(kind)
        elif kind == "CMD":
            self.media.execute(value)
        elif kind == "LINK":
            self._on_link(value == "UP")
        elif kind == "STATUS":
            parsed = {}
            for kv in value.split(","):
                if "=" in kv:
                    k, _, v = kv.partition("=")
                    parsed[k.strip()] = v.strip()
            if "quality" in parsed:
                try:
                    self.last_quality = int(parsed["quality"])
                except ValueError:
                    pass
            if "batt" in parsed and parsed["batt"] != "255":
                self.last_battery = parsed["batt"]
            if "vol" in parsed:
                self.last_voltage = parsed["vol"]
            log.info("Remote status: %s", value)
            self.tray.set_status(self._status_text())
        elif kind == "HEARTBEAT":
            log.debug("Heartbeat from receiver")
        elif kind == "PONG":
            log.debug("Receiver answered PING")
        elif kind == "INFO":
            log.info("Receiver info: %s", value)
        elif kind == "WARN":
            log.warning("Receiver warning: %s", value)
        elif kind == "READY":
            log.info("Receiver ready")
        else:
            log.debug("Unparsed line: %s", line)

    # ------------------------------------------------------------ tray actions

    def _on_quit(self):
        log.info("Quit requested via tray")
        self.quit_event.set()

    def _on_reconnect(self):
        log.info("Manual reconnect requested")
        self.serial.reconnect()

    def _on_toggle_notifications(self):
        self.settings["notifications"] = not bool(self.settings.get("notifications", True))
        log.info("Notifications %s", "enabled" if self.settings["notifications"] else "disabled")

    def _on_toggle_startup(self):
        want = not get_start_with_windows()
        ok = set_start_with_windows(want)
        self.settings["start_with_windows"] = want
        log.info("Start with Windows set to %s (%s)",
                 want, "ok" if ok else "failed")

    # ---------------------------------------------------------- settings window

    def _open_settings_window(self):
        self._ui_queue.put(self._build_settings_window)

    def _ui_worker(self):
        while True:
            task = self._ui_queue.get()
            if task is None:
                break
            try:
                task()
            except Exception:
                log.exception("Settings window error")

    def _build_settings_window(self):
        dark = bool(self.settings.get("dark_mode", True))
        bg, fg, card, btn = (("#1e1e1e", "#e8e8e8", "#2d2d30", "#3c3c41") if dark
                             else ("#f5f5f5", "#111111", "#ffffff", "#e0e0e0"))

        root = tk.Tk()
        root.title("Spotify Remote Settings")
        root.configure(bg=bg)
        root.geometry("440x420")
        root.resizable(False, False)

        def label(parent, text):
            return tk.Label(parent, text=text, bg=bg, fg=fg, anchor="w",
                            font=("Segoe UI", 10))

        pad = dict(pady=(10, 2))

        tk.Label(root, text="Spotify Remote", bg=bg, fg="#1DB954",
                 font=("Segoe UI", 14, "bold")).pack(**pad)

        # --- COM port -----------------------------------------------------
        frame_port = tk.Frame(root, bg=bg)
        frame_port.pack(fill="x", padx=20, pady=(10, 0))
        label(frame_port, "Receiver COM port (AUTO = detect):").pack(fill="x")
        row = tk.Frame(frame_port, bg=bg)
        row.pack(fill="x", pady=4)
        self._port_var = tk.StringVar(value=self.settings.get("com_port", "AUTO"))

        def refresh_ports():
            try:
                import serial.tools.list_ports
                ports = [p.device for p in serial.tools.list_ports.comports()]
            except Exception:
                ports = []
            combo["values"] = ["AUTO"] + ports
            if self._port_var.get() not in combo["values"]:
                self._port_var.set("AUTO")

        combo = ttk.Combobox(row, textvariable=self._port_var, values=["AUTO"],
                             state="readonly", width=18)
        combo.pack(side="left")
        ttk.Button(row, text="Refresh", command=refresh_ports).pack(side="left", padx=6)

        # --- options -------------------------------------------------------
        frame_opt = tk.Frame(root, bg=bg)
        frame_opt.pack(fill="x", padx=20, pady=(12, 0))
        self._var_notify = tk.BooleanVar(value=bool(self.settings.get("notifications", True)))
        self._var_dark = tk.BooleanVar(value=dark)
        self._var_startup = tk.BooleanVar(value=get_start_with_windows())
        self._var_hidden = tk.BooleanVar(value=bool(self.settings.get("start_hidden", False)))

        def cb(text, var):
            return tk.Checkbutton(frame_opt, text=text, variable=var, bg=bg, fg=fg,
                                  selectcolor=card, activebackground=bg,
                                  activeforeground=fg, font=("Segoe UI", 10),
                                  anchor="w", width=36)

        cb("Show notifications (balloon popups)", self._var_notify).pack(anchor="w")
        cb("Dark mode (applies on next open)", self._var_dark).pack(anchor="w")
        cb("Start hidden (tray only, no window)", self._var_hidden).pack(anchor="w")
        cb("Start with Windows (tray menu also toggles this)", self._var_startup).pack(anchor="w")

        # --- actions -------------------------------------------------------
        frame_act = tk.Frame(root, bg=bg)
        frame_act.pack(fill="x", padx=20, pady=(16, 4))

        def save():
            self.settings["com_port"] = self._port_var.get()
            self.settings["notifications"] = bool(self._var_notify.get())
            self.settings["dark_mode"] = bool(self._var_dark.get())
            self.settings["start_hidden"] = bool(self._var_hidden.get())
            set_start_with_windows(bool(self._var_startup.get()))
            self.settings["start_with_windows"] = bool(self._var_startup.get())
            if self.settings["com_port"] != self.settings.data.get("_old_port"):
                self.serial.reconnect()
            log.info("Settings saved")
            root.destroy()

        def reconnect_now():
            log.info("Reconnect pressed in settings window")
            self.serial.reconnect()

        ttk.Button(frame_act, text="Save & Apply", command=save).pack(side="left", padx=4)
        ttk.Button(frame_act, text="Reconnect Now", command=reconnect_now).pack(side="left", padx=4)
        ttk.Button(frame_act, text="Cancel", command=root.destroy).pack(side="left", padx=4)

        self.settings.data["_old_port"] = self.settings.get("com_port", "AUTO")
        refresh_ports()
        root.mainloop()


def main():
    parser = argparse.ArgumentParser(description="Spotify Remote - Windows media key bridge")
    parser.add_argument("--hidden", action="store_true", help="start without opening a window")
    parser.add_argument("--port", help="force COM port (overrides config.json)")
    args = parser.parse_args()

    if _is_another_instance(__file__):
        print("Spotify Remote is already running. Check the system tray.")
        return 1

    settings = Settings()
    if args.port:
        settings["com_port"] = args.port

    log = setup_logging(settings.get("log_level", "INFO"))
    log.info("=" * 64)
    log.info("Spotify Remote starting (hidden=%s, port=%s)",
             args.hidden, settings.get("com_port"))

    app = App(settings, args.hidden)
    app.start()

    if not args.hidden and not settings.get("start_hidden", False):
        app._open_settings_window()

    try:
        app.quit_event.wait()
    except KeyboardInterrupt:
        pass

    log.info("Shutting down")
    app.serial.stop()
    app.tray.stop()
    app._ui_queue.put(None)
    return 0


if __name__ == "__main__":
    sys.exit(main())
