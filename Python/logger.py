"""
logger.py - Logging setup for the Spotify Remote desktop app.

Writes to logs/daily.log with daily rotation (kept for 7 days) plus a
console mirror. The log captures: time, received packets, signal quality,
errors, reconnects and executed commands (as required by the project spec).
"""

import logging
from logging.handlers import TimedRotatingFileHandler
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent
LOG_DIR = BASE_DIR / "logs"

_formatter = logging.Formatter(
    fmt="[%(asctime)s] %(levelname)-7s %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)


def setup_logging(level: str = "INFO") -> logging.Logger:
    """Configure and return the shared 'spotify_remote' logger (idempotent)."""
    LOG_DIR.mkdir(exist_ok=True)
    root = logging.getLogger("spotify_remote")
    if not root.handlers:
        file_handler = TimedRotatingFileHandler(
            LOG_DIR / "daily.log",
            when="midnight",
            backupCount=7,
            encoding="utf-8",
        )
        file_handler.suffix = "%Y-%m-%d"
        file_handler.setFormatter(_formatter)

        console = logging.StreamHandler()
        console.setFormatter(_formatter)

        root.addHandler(file_handler)
        root.addHandler(console)
    root.setLevel(level.upper())
    return root
