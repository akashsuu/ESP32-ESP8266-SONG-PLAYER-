"""
settings.py - Configuration storage and Windows startup integration.

Config is persisted as JSON next to this file (config.json). The
"Start with Windows" option is stored in HKCU Run registry key so it
works without administrator rights.
"""

import json
import logging
import os
import sys
from pathlib import Path

log = logging.getLogger("spotify_remote")

BASE_DIR = Path(__file__).resolve().parent
CONFIG_PATH = BASE_DIR / "config.json"

DEFAULTS = {
    "com_port": "AUTO",
    "baudrate": 115200,
    "reconnect_interval_s": 3,
    "notifications": True,
    "dark_mode": True,
    "start_with_windows": False,
    "start_hidden": False,
    "log_level": "INFO",
    "device_id": "A1B2",
    "encryption_key": "1C9B4FE72A88D35E710FB6C4396DA2F5",
}

_RUN_KEY = r"Software\Microsoft\Windows\CurrentVersion\Run"
_APP_NAME = "SpotifyRemote"


class Settings:
    """Thin dict-like wrapper around config.json with auto-save."""

    def __init__(self):
        self.data = dict(DEFAULTS)
        self.load()

    def load(self):
        try:
            with open(CONFIG_PATH, "r", encoding="utf-8") as f:
                stored = json.load(f)
            for key in DEFAULTS:
                if key in stored:
                    self.data[key] = stored[key]
        except (OSError, ValueError) as exc:
            log.warning("Could not read config.json (%s); using defaults", exc)

    def save(self):
        try:
            with open(CONFIG_PATH, "w", encoding="utf-8") as f:
                json.dump(self.data, f, indent=2)
        except OSError as exc:
            log.error("Failed to save config.json: %s", exc)

    def get(self, key, default=None):
        return self.data.get(key, default)

    def __getitem__(self, key):
        return self.data[key]

    def __setitem__(self, key, value):
        self.data[key] = value
        self.save()


# --------------------------------------------------------------------------
# Windows startup registry helpers (HKCU, no admin required)
# --------------------------------------------------------------------------

def _pythonw_exe() -> str:
    """Return pythonw.exe when available so startup is windowless."""
    exe = sys.executable
    if exe.lower().endswith("python.exe"):
        alt = exe[:-4] + "w.exe"
        if os.path.exists(alt):
            return alt
    return exe


def get_start_with_windows() -> bool:
    try:
        import winreg
        key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, _RUN_KEY)
        try:
            winreg.QueryValueEx(key, _APP_NAME)
            return True
        except OSError:
            return False
        finally:
            winreg.CloseKey(key)
    except OSError:
        return False


def set_start_with_windows(enabled: bool) -> bool:
    try:
        import winreg
        key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, _RUN_KEY, 0, winreg.KEY_SET_VALUE)
        try:
            if enabled:
                cmd = f'"{_pythonw_exe()}" "{str(BASE_DIR / "main.py")}" --hidden'
                winreg.SetValueEx(key, _APP_NAME, 0, winreg.REG_SZ, cmd)
            else:
                try:
                    winreg.DeleteValue(key, _APP_NAME)
                except OSError:
                    pass
            return True
        finally:
            winreg.CloseKey(key)
    except OSError as exc:
        log.error("Registry update failed: %s", exc)
        return False
