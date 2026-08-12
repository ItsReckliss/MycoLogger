"""Extract capture timestamps from uploaded image metadata."""

from __future__ import annotations

from dataclasses import dataclass
from datetime import UTC, datetime, timedelta, timezone
from io import BytesIO
from zoneinfo import ZoneInfo, ZoneInfoNotFoundError

from PIL import Image, UnidentifiedImageError


EXIF_DATETIME_TAGS = (36867, 36868, 306)  # Original, digitized, modified.
EXIF_OFFSET_TAGS = (36881, 36882, 36880)


@dataclass(frozen=True)
class CaptureTime:
    utc: datetime
    local_date: str
    source: str


def _text(value: object) -> str | None:
    if isinstance(value, bytes):
        return value.decode("ascii", errors="ignore").strip("\x00 ") or None
    if isinstance(value, str):
        return value.strip("\x00 ") or None
    return None


def _offset(value: object) -> timezone | None:
    text = _text(value)
    if text is None or len(text) != 6 or text[0] not in "+-" or text[3] != ":":
        return None
    try:
        hours = int(text[1:3])
        minutes = int(text[4:6])
    except ValueError:
        return None
    if hours > 23 or minutes > 59:
        return None
    delta = timedelta(hours=hours, minutes=minutes)
    return timezone(delta if text[0] == "+" else -delta)


def extract_capture_time(content: bytes, local_timezone: str) -> CaptureTime | None:
    """Return EXIF capture time, assuming configured local time if no offset exists."""
    try:
        with Image.open(BytesIO(content)) as image:
            exif = image.getexif()
    except (UnidentifiedImageError, OSError, ValueError):
        return None

    for index, tag in enumerate(EXIF_DATETIME_TAGS):
        value = _text(exif.get(tag))
        if value is None:
            continue
        try:
            naive = datetime.strptime(value, "%Y:%m:%d %H:%M:%S")
        except ValueError:
            continue
        offset = _offset(exif.get(EXIF_OFFSET_TAGS[index]))
        try:
            assumed_zone = ZoneInfo(local_timezone)
        except ZoneInfoNotFoundError:
            assumed_zone = UTC
        aware = naive.replace(tzinfo=offset or assumed_zone)
        return CaptureTime(
            utc=aware.astimezone(UTC),
            local_date=naive.date().isoformat(),
            source="exif",
        )
    return None
