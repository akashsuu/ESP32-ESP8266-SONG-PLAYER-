"""
tray.py - System tray integration (pystray + Pillow).

Provides:
  - a dark Spotify-style tray icon
  - live tooltip status (connection state, battery, signal)
  - menu: status, reconnect, settings, notifications toggle,
    start-with-Windows toggle, quit
  - balloon notifications via pystray's native notify()
"""

import pystray
from PIL import Image, ImageDraw


def create_icon_image():
    """64x64 dark rounded square with a green play button."""
    img = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([0, 0, 63, 63], radius=14, fill=(24, 26, 32, 255))
    d.ellipse([13, 13, 51, 51], fill=(29, 185, 84, 255))
    d.polygon([(27, 21), (27, 43), (46, 32)], fill=(255, 255, 255, 255))
    return img


class TrayManager:
    """Owns the pystray icon; callbacks are invoked from the tray thread."""

    def __init__(self, on_quit, on_reconnect, on_open_settings,
                 on_toggle_notifications, on_toggle_startup,
                 status_provider, notifications_provider, startup_provider):
        self._on_quit = on_quit
        self._on_reconnect = on_reconnect
        self._on_open_settings = on_open_settings
        self._on_toggle_notifications = on_toggle_notifications
        self._on_toggle_startup = on_toggle_startup
        self._status_provider = status_provider
        self._notifications_provider = notifications_provider
        self._startup_provider = startup_provider
        self._icon = None

    # ------------------------------------------------------------ lifecycle

    def run(self):
        """Blocking; run inside its own thread."""
        menu = pystray.Menu(
            pystray.MenuItem(
                lambda item: self._status_provider(),
                lambda icon, item: None,
                enabled=False,
            ),
            pystray.Menu.SEPARATOR,
            pystray.MenuItem(
                "Reconnect",
                lambda icon, item: self._on_reconnect(),
            ),
            pystray.MenuItem(
                "Settings...",
                lambda icon, item: self._on_open_settings(),
            ),
            pystray.MenuItem(
                "Notifications",
                lambda icon, item: self._on_toggle_notifications(),
                checked=lambda item: bool(self._notifications_provider()),
            ),
            pystray.MenuItem(
                "Start with Windows",
                lambda icon, item: self._on_toggle_startup(),
                checked=lambda item: bool(self._startup_provider()),
            ),
            pystray.Menu.SEPARATOR,
            pystray.MenuItem(
                "Quit",
                lambda icon, item: self._on_quit(),
            ),
        )
        self._icon = pystray.Icon("SpotifyRemote", create_icon_image(),
                                  "Spotify Remote", menu)
        self._icon.run()

    def stop(self):
        if self._icon is not None:
            try:
                self._icon.stop()
            except Exception:
                pass

    # --------------------------------------------------------------- status

    def set_status(self, text: str):
        if self._icon is not None:
            try:
                self._icon.title = text
            except Exception:
                pass

    def notify(self, title: str, message: str):
        """Native balloon notification (safe to call from other threads)."""
        if self._icon is None:
            return
        try:
            self._icon.notify(message, title)
        except Exception:
            pass
