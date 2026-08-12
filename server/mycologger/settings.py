"""Cross-platform runtime settings for the MycoLogger server."""

from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path


SERVER_DIR = Path(__file__).resolve().parent.parent


@dataclass(frozen=True)
class Settings:
    database_path: Path
    photo_directory: Path
    host: str
    port: int
    receiver_enabled: bool
    receiver_port: str | None
    receiver_baud: int
    receiver_retry_seconds: float
    local_timezone: str


def _environment_flag(name: str, default: bool) -> bool:
    value = os.environ.get(name)
    if value is None:
        return default
    return value.strip().casefold() not in {"0", "false", "no", "off"}


def load_settings() -> Settings:
    default_database = SERVER_DIR / "data" / "mycologger.sqlite3"
    database_path = Path(
        os.environ.get("MYCOLOGGER_DATABASE_PATH", str(default_database))
    ).expanduser()
    return Settings(
        database_path=database_path,
        photo_directory=Path(
            os.environ.get(
                "MYCOLOGGER_PHOTO_DIRECTORY",
                str(database_path.parent / "photos"),
            )
        ).expanduser(),
        host=os.environ.get("MYCOLOGGER_HOST", "127.0.0.1"),
        port=int(os.environ.get("MYCOLOGGER_PORT", "8080")),
        receiver_enabled=_environment_flag("MYCOLOGGER_RECEIVER_ENABLED", True),
        receiver_port=os.environ.get("MYCOLOGGER_RECEIVER_PORT") or None,
        receiver_baud=int(os.environ.get("MYCOLOGGER_RECEIVER_BAUD", "115200")),
        receiver_retry_seconds=float(
            os.environ.get("MYCOLOGGER_RECEIVER_RETRY_SECONDS", "1.0")
        ),
        local_timezone=os.environ.get(
            "MYCOLOGGER_LOCAL_TIMEZONE", "America/New_York"
        ),
    )
