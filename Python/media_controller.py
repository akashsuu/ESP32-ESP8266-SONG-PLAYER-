"""
media_controller.py - Maps receiver commands to Windows multimedia keys.

The `keyboard` library uses SendInput under the hood, so these key
combinations control any media app that registers the Windows media keys
(Spotify, YouTube Music, VLC, Windows Media Player, ...).

Mapping (per project spec):
    NEXT     -> Media Next Track
    PREVIOUS -> Media Previous Track
    PLAY     -> Media Play/Pause
    VOLUP    -> Volume Up
    VOLDOWN  -> Volume Down
    MUTE     -> Volume Mute
"""

import logging
import time

log = logging.getLogger("spotify_remote")

try:
    import keyboard
except ImportError:  # allow import for linting on non-Windows boxes
    keyboard = None

MEDIA_KEYS = {
    "NEXT": "next track",
    "PREVIOUS": "previous track",
    "PLAY": "play/pause media",
    "VOLUP": "volume up",
    "VOLDOWN": "volume down",
    "MUTE": "volume mute",
}


class MediaController:
    """Executes media commands with a single retry for robustness."""

    def __init__(self):
        self.last_command = None
        self.last_at = None

    def execute(self, command: str) -> bool:
        cmd = command.strip().upper()
        key = MEDIA_KEYS.get(cmd)
        if keyboard is None:
            log.error("The 'keyboard' library is not installed")
            return False
        if key is None:
            log.warning("Unknown command ignored: %r", command)
            return False

        for attempt in (1, 2):
            try:
                keyboard.send(key)
                self.last_command = cmd
                self.last_at = time.time()
                log.info("Executed %s -> %s", cmd, key)
                return True
            except Exception as exc:
                log.error("Failed to send %r (attempt %d/2): %s", key, attempt, exc)
                time.sleep(0.1)
        return False
