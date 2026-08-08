"""
serial_manager.py - USB serial connection to the ESP32 Receiver.

Threaded connection manager with:
  - automatic receiver discovery (known VID/PID or name heuristics)
  - automatic reconnect with configurable interval
  - line-oriented protocol parsing (one message per line)
  - manual reconnect requests and clean shutdown
"""

import logging
import threading

import serial
import serial.tools.list_ports

log = logging.getLogger("spotify_remote")

# Known USB bridge / native USB identifiers used by typical ESP32 dev kits.
KNOWN_VID_PID = {
    (0x10C4, 0xEA60),  # Silicon Labs CP2102 / CP2104 / CP210x
    (0x1A86, 0x7523),  # WCH CH340 / CH341
    (0x303A, 0x1001),  # Espressif ESP32 (native USB CDC)
    (0x303A, 0x1002),  # Espressif ESP32-S3 (native USB CDC)
    (0x2E8A, 0x0005),  # Raspberry Pi Pico / RP2040 CDC
    (0x2341, 0x0043),  # Arduino Leonardo / generic CDC
    (0x0403, 0x6001),  # FTDI FT232R
}


class SerialManager(threading.Thread):
    """Owns the COM port; delivers decoded lines via callbacks."""

    def __init__(self, settings, on_message, on_link):
        super().__init__(name="serial-manager", daemon=True)
        self._settings = settings
        self._on_message = on_message       # on_message(line: str)
        self._on_link = on_link             # on_link(up: bool)
        self._stop = threading.Event()
        self._force_reconnect = threading.Event()
        self._write_lock = threading.Lock()
        self._ser = None
        self._port = None

    # ------------------------------------------------------------------ API

    def stop(self):
        self._stop.set()
        self._force_reconnect.set()  # wake the run loop

    def reconnect(self):
        """Close the current port; the run loop reconnects immediately."""
        log.info("Reconnect requested")
        self._force_reconnect.set()

    def send(self, line: str) -> bool:
        if not self._ser:
            return False
        try:
            with self._write_lock:
                self._ser.write((line + "\n").encode("utf-8"))
                self._ser.flush()
            return True
        except serial.SerialException as exc:
            log.error("Serial write failed: %s", exc)
            return False

    # ------------------------------------------------------------- internals

    def _find_port(self):
        """Return the device path of a plausible ESP32 receiver, or None."""
        try:
            ports = list(serial.tools.list_ports.comports())
        except Exception as exc:
            log.error("Port enumeration failed: %s", exc)
            return None

        for p in ports:
            if p.vid is not None and (p.vid, p.pid) in KNOWN_VID_PID:
                return p.device
        for p in ports:
            name = f"{p.name or ''} {p.description or ''}".lower()
            if any(k in name for k in ("esp32", "usbserial", "usb serial", "ch340", "cp210", "ft232")):
                return p.device
        return None

    def _open(self, port: str):
        self._ser = serial.Serial(
            port=port,
            baudrate=int(self._settings.get("baudrate", 115200)),
            timeout=0.3,
        )
        self._port = port
        log.info("Connected to receiver on %s @ %d baud", port, self._ser.baudrate)
        self._on_link(True)

    def _close(self):
        if self._ser is not None:
            try:
                self._ser.close()
            except Exception:
                pass
            self._ser = None
        self._port = None

    def _read_loop(self):
        while not self._stop.is_set() and not self._force_reconnect.is_set():
            try:
                raw = self._ser.readline()
            except serial.SerialException:
                return
            if not raw:
                continue
            try:
                line = raw.decode("utf-8", errors="replace").strip()
            except Exception:
                continue
            if line:
                self._on_message(line)

    def run(self):
        while not self._stop.is_set():
            self._force_reconnect.clear()

            port = str(self._settings.get("com_port", "AUTO")).strip()
            if not port or port.upper() == "AUTO":
                port = self._find_port()
                if port:
                    log.info("Auto-detected receiver on %s", port)

            if not port:
                log.warning("No ESP32 receiver found; retrying...")
                self._stop.wait(self._settings.get("reconnect_interval_s", 3))
                continue

            try:
                self._open(port)
                self._read_loop()
            except serial.SerialException as exc:
                log.error("Serial error on %s: %s", port, exc)
            except Exception:
                log.exception("Unexpected error on %s", port)
            finally:
                was_connected = self._ser is not None
                self._close()
                if was_connected:
                    self._on_link(False)

            if self._stop.is_set():
                break

            interval = self._settings.get("reconnect_interval_s", 3)
            log.info("Reconnecting in %s s...", interval)
            self._stop.wait(interval)

        log.info("Serial manager stopped")
