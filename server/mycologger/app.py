"""FastAPI application for the MycoLogger dashboard and API."""

from __future__ import annotations

from contextlib import asynccontextmanager
from datetime import UTC, date, datetime, time
from pathlib import Path
import sqlite3
from uuid import uuid4
from zoneinfo import ZoneInfo

from fastapi import FastAPI, HTTPException, Query, Request
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel, Field

from . import __version__
from .database import (
    add_spawn_jar_photo,
    add_grow_photo,
    cancel_node_reservation,
    complete_node_provisioning,
    create_spawn_jar,
    create_spawn_jars,
    delete_spawn_jar_photo,
    delete_grow_photo,
    get_current_grows,
    get_counts,
    get_grow,
    get_grow_photo,
    get_provisioned_transmitters,
    get_provisioning_status,
    get_spawn_jar,
    get_spawn_jar_photo,
    get_spawn_jars,
    get_photos_needing_metadata,
    get_node,
    get_nodes,
    get_tubs,
    initialize,
    reserve_node_id,
    set_spawn_jar_locked,
    spawn_jar_groups_to_tubs,
    spawn_jar_to_tub,
    spawn_jars_to_tub,
    update_grow,
    update_grow_photo_capture_metadata,
    update_node_settings,
    update_spawn_jar,
)
from .receiver import ReceiverService
from .settings import load_settings
from .photo_metadata import CaptureTime, extract_capture_time


STATIC_DIR = Path(__file__).resolve().parent / "static"
settings = load_settings()
receiver_service = ReceiverService(
    settings.database_path,
    enabled=settings.receiver_enabled,
    requested_port=settings.receiver_port,
    baud=settings.receiver_baud,
    retry_seconds=settings.receiver_retry_seconds,
)


class NodeSettingsUpdate(BaseModel):
    name: str = Field(min_length=1, max_length=64)
    tub_name: str = Field(default="", max_length=64)
    location: str = Field(default="", max_length=128)
    notes: str = Field(default="", max_length=2000)
    active: bool = True
    report_interval_s: int = Field(ge=15, le=604800)


class GrowUpdate(BaseModel):
    name: str = Field(min_length=1, max_length=64)
    species: str = Field(default="", max_length=96)
    strain: str = Field(default="", max_length=96)
    stage: str = Field(
        default="colonizing",
        pattern="^(colonizing|fruiting|harvesting|complete|paused)$",
    )
    spawn_to_bulk_on: date | None = None
    spawn_ratio: str = Field(default="", max_length=32)
    completed_on: date | None = None
    pin_dates: list[date] = Field(default_factory=list, max_length=50)
    notes: str = Field(default="", max_length=10000)
    active: bool = True


class SpawnJarUpdate(BaseModel):
    name: str = Field(min_length=1, max_length=64)
    grain_type: str = Field(default="", max_length=96)
    prep_tek: str = Field(default="", max_length=5000)
    pressure_cooker_minutes: int | None = Field(default=None, ge=0, le=1440)
    pressure_psi: float | None = Field(default=None, ge=0, le=100)
    dry_grain_grams_per_jar: float | None = Field(default=None, ge=0, le=100000)
    jar_count: int = Field(default=1, ge=1, le=1000)
    pressure_cooked_on: date | None = None
    inoculated_on: date | None = None
    culture: str = Field(default="", max_length=128)
    species: str = Field(default="", max_length=128)
    break_shake_dates: list[date] = Field(default_factory=list, max_length=50)
    notes: str = Field(default="", max_length=10000)


class SpawnJarLockUpdate(BaseModel):
    locked: bool


class SpawnJarBulkCreate(SpawnJarUpdate):
    quantity: int = Field(default=1, ge=1, le=100)


class SpawnJarToTub(BaseModel):
    tub_name: str = Field(min_length=1, max_length=64)
    node_id: int | None = Field(default=None, ge=1)
    species: str = Field(default="", max_length=96)
    strain: str = Field(default="", max_length=96)
    spawn_to_bulk_on: date
    spawn_ratio: str = Field(default="Not recorded", min_length=1, max_length=32)
    notes: str = Field(default="", max_length=10000)


class SpawnJarsToTub(SpawnJarToTub):
    jar_ids: list[int] = Field(min_length=1, max_length=100)


class SpawnJarGroup(BaseModel):
    jar_ids: list[int] = Field(min_length=1, max_length=100)
    tub_name: str = Field(min_length=1, max_length=64)
    node_id: int | None = Field(default=None, ge=1)
    spawn_to_bulk_on: date
    spawn_ratio: str = Field(min_length=1, max_length=32)
    notes: str = Field(default="", max_length=10000)


class SpawnJarGroups(BaseModel):
    groups: list[SpawnJarGroup] = Field(min_length=1, max_length=100)


class ProvisioningReservationRequest(BaseModel):
    hardware_uid: str = Field(min_length=24, max_length=24)
    requested_node_id: int | None = Field(default=None, ge=1, le=4294967294)


class ProvisioningReservationAction(BaseModel):
    hardware_uid: str = Field(min_length=24, max_length=24)
    reservation_token: str = Field(min_length=32, max_length=64)


@asynccontextmanager
async def lifespan(_: FastAPI):
    initialize(settings.database_path)
    settings.photo_directory.mkdir(parents=True, exist_ok=True)
    for photo in get_photos_needing_metadata(settings.database_path):
        path = settings.photo_directory / str(photo["stored_name"])
        if not path.is_file():
            continue
        try:
            capture = extract_capture_time(path.read_bytes(), settings.local_timezone)
            if capture is not None:
                update_grow_photo_capture_metadata(
                    settings.database_path,
                    int(photo["photo_id"]),
                    taken_on=capture.local_date,
                    taken_at_utc=capture.utc.isoformat(),
                    capture_time_source=capture.source,
                )
        except (OSError, ValueError):
            # A corrupt legacy file should not prevent the receiver/server starting.
            continue
    receiver_service.start()
    try:
        yield
    finally:
        receiver_service.stop()


app = FastAPI(
    title="MycoLogger",
    version=__version__,
    lifespan=lifespan,
    docs_url="/api/docs",
    redoc_url=None,
)
app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")


@app.get("/", include_in_schema=False)
def dashboard() -> FileResponse:
    return FileResponse(
        STATIC_DIR / "index.html",
        headers={"Cache-Control": "no-store, max-age=0"},
    )


@app.get("/api/health")
def health() -> dict[str, object]:
    return {
        "status": "ok",
        "service": "mycologger",
        "version": __version__,
        "time_utc": datetime.now(UTC).isoformat(),
        "database_ready": settings.database_path.exists(),
        "receiver": receiver_service.snapshot(),
    }


@app.get("/api/dashboard")
def dashboard_data() -> dict[str, object]:
    nodes = get_nodes(settings.database_path)
    return {
        "generated_utc": datetime.now(UTC).isoformat(),
        "receiver": receiver_service.snapshot(),
        "counts": get_counts(settings.database_path),
        "nodes": nodes,
    }


@app.get("/api/nodes")
def nodes() -> dict[str, object]:
    return {"nodes": get_nodes(settings.database_path)}


def _normalize_hardware_uid(value: str) -> str:
    normalized = "".join(character for character in value.upper() if character in "0123456789ABCDEF")
    if len(normalized) != 24:
        raise HTTPException(
            status_code=422,
            detail="Hardware UID must contain exactly 24 hexadecimal characters",
        )
    return normalized


@app.get("/api/provisioning/status")
def provisioning_status(hardware_uid: str | None = None) -> dict[str, object]:
    normalized = (
        _normalize_hardware_uid(hardware_uid) if hardware_uid is not None else None
    )
    return get_provisioning_status(settings.database_path, normalized)


@app.get("/api/provisioning/transmitters")
def provisioned_transmitters() -> dict[str, object]:
    return {"transmitters": get_provisioned_transmitters(settings.database_path)}


@app.post("/api/provisioning/reservations", status_code=201)
def create_provisioning_reservation(
    request: ProvisioningReservationRequest,
) -> dict[str, object]:
    hardware_uid = _normalize_hardware_uid(request.hardware_uid)
    try:
        reservation = reserve_node_id(
            settings.database_path,
            hardware_uid=hardware_uid,
            reservation_token=uuid4().hex,
            requested_node_id=request.requested_node_id,
            ttl_seconds=5 * 60,
        )
    except ValueError as exc:
        raise HTTPException(status_code=409, detail=str(exc)) from exc
    return {"reservation": reservation}


@app.post("/api/provisioning/complete")
def complete_provisioning(
    request: ProvisioningReservationAction,
) -> dict[str, object]:
    hardware_uid = _normalize_hardware_uid(request.hardware_uid)
    try:
        transmitter = complete_node_provisioning(
            settings.database_path,
            reservation_token=request.reservation_token,
            hardware_uid=hardware_uid,
        )
    except KeyError as exc:
        raise HTTPException(status_code=404, detail="Reservation not found") from exc
    except ValueError as exc:
        raise HTTPException(status_code=409, detail=str(exc)) from exc
    return {"transmitter": transmitter}


@app.post("/api/provisioning/cancel")
def cancel_provisioning(
    request: ProvisioningReservationAction,
) -> dict[str, object]:
    hardware_uid = _normalize_hardware_uid(request.hardware_uid)
    cancelled = cancel_node_reservation(
        settings.database_path,
        reservation_token=request.reservation_token,
        hardware_uid=hardware_uid,
    )
    return {"cancelled": cancelled}


@app.get("/api/nodes/{node_id}")
def node(node_id: int) -> dict[str, object]:
    result = get_node(settings.database_path, node_id)
    if result is None:
        raise HTTPException(status_code=404, detail="Node not found")
    return {"node": result}


@app.put("/api/nodes/{node_id}")
def save_node(node_id: int, update: NodeSettingsUpdate) -> dict[str, object]:
    try:
        result = update_node_settings(
            settings.database_path,
            node_id,
            name=update.name,
            tub_name=update.tub_name,
            location=update.location,
            notes=update.notes,
            active=update.active,
            report_interval_s=update.report_interval_s,
        )
    except KeyError as exc:
        raise HTTPException(status_code=404, detail="Node not found") from exc
    return {"node": result}


@app.get("/api/tubs")
def tubs() -> dict[str, object]:
    return {"tubs": get_tubs(settings.database_path)}


@app.get("/api/grows")
def current_grows(hours: int = Query(default=72, ge=1, le=744)) -> dict[str, object]:
    return {"grows": get_current_grows(settings.database_path, hours=hours)}


@app.get("/api/grows/{tub_id}")
def grow(tub_id: int, hours: int = Query(default=72, ge=1, le=744)) -> dict[str, object]:
    result = get_grow(settings.database_path, tub_id, hours=hours)
    if result is None:
        raise HTTPException(status_code=404, detail="Current grow not found")
    return {"grow": result}


@app.put("/api/grows/{tub_id}")
def save_grow(tub_id: int, update: GrowUpdate) -> dict[str, object]:
    try:
        result = update_grow(
            settings.database_path,
            tub_id,
            name=update.name,
            species=update.species,
            strain=update.strain,
            stage=update.stage,
            spawn_to_bulk_on=(
                update.spawn_to_bulk_on.isoformat()
                if update.spawn_to_bulk_on is not None
                else None
            ),
            completed_on=(
                update.completed_on.isoformat()
                if update.completed_on is not None
                else None
            ),
            pin_dates=[item.isoformat() for item in update.pin_dates],
            notes=update.notes,
            active=update.active,
            spawn_ratio=update.spawn_ratio,
        )
    except KeyError as exc:
        raise HTTPException(status_code=404, detail="Grow not found") from exc
    except sqlite3.IntegrityError as exc:
        raise HTTPException(status_code=409, detail="That grow name is already in use") from exc
    return {"grow": result}


def _spawn_jar_values(update: SpawnJarUpdate) -> dict[str, object]:
    return {
        "name": update.name,
        "grain_type": update.grain_type,
        "prep_tek": update.prep_tek,
        "pressure_cooker_minutes": update.pressure_cooker_minutes,
        "pressure_psi": update.pressure_psi,
        "dry_grain_grams_per_jar": update.dry_grain_grams_per_jar,
        "jar_count": update.jar_count,
        "pressure_cooked_on": (
            update.pressure_cooked_on.isoformat()
            if update.pressure_cooked_on is not None else None
        ),
        "inoculated_on": (
            update.inoculated_on.isoformat()
            if update.inoculated_on is not None else None
        ),
        "culture": update.culture,
        "species": update.species,
        "break_shake_dates": [item.isoformat() for item in update.break_shake_dates],
        "notes": update.notes,
    }


@app.get("/api/jars")
def spawn_jars(status: str | None = Query(default="active")) -> dict[str, object]:
    if status not in {"active", "spawned", "archived", "all", None}:
        raise HTTPException(status_code=400, detail="Invalid jar status")
    return {
        "jars": get_spawn_jars(
            settings.database_path, status=None if status == "all" else status
        )
    }


@app.post("/api/jars", status_code=201)
def new_spawn_jar(update: SpawnJarUpdate) -> dict[str, object]:
    try:
        result = create_spawn_jar(settings.database_path, **_spawn_jar_values(update))
    except sqlite3.IntegrityError as exc:
        raise HTTPException(status_code=409, detail="That jar name is already in use") from exc
    return {"jar": result}


@app.post("/api/jars/bulk", status_code=201)
def new_spawn_jars(update: SpawnJarBulkCreate) -> dict[str, object]:
    values = _spawn_jar_values(update)
    values.pop("jar_count", None)
    try:
        results = create_spawn_jars(
            settings.database_path,
            quantity=update.quantity,
            **values,
        )
    except sqlite3.IntegrityError as exc:
        raise HTTPException(
            status_code=409,
            detail="One or more generated jar names are already in use",
        ) from exc
    return {"jars": results}


@app.get("/api/jars/{jar_id}")
def spawn_jar(jar_id: int) -> dict[str, object]:
    result = get_spawn_jar(settings.database_path, jar_id)
    if result is None:
        raise HTTPException(status_code=404, detail="Spawn jar not found")
    return {"jar": result}


@app.put("/api/jars/{jar_id}")
def save_spawn_jar(jar_id: int, update: SpawnJarUpdate) -> dict[str, object]:
    try:
        result = update_spawn_jar(
            settings.database_path, jar_id, **_spawn_jar_values(update)
        )
    except KeyError as exc:
        raise HTTPException(status_code=404, detail="Spawn jar not found") from exc
    except PermissionError as exc:
        raise HTTPException(status_code=423, detail=str(exc)) from exc
    except sqlite3.IntegrityError as exc:
        raise HTTPException(status_code=409, detail="That jar name is already in use") from exc
    return {"jar": result}


@app.put("/api/jars/{jar_id}/lock")
def lock_spawn_jar(jar_id: int, update: SpawnJarLockUpdate) -> dict[str, object]:
    try:
        result = set_spawn_jar_locked(
            settings.database_path, jar_id, locked=update.locked
        )
    except KeyError as exc:
        raise HTTPException(status_code=404, detail="Spawn jar not found") from exc
    return {"jar": result}


@app.post("/api/jars/{jar_id}/spawn", status_code=201)
def roll_spawn_jar_into_tub(
    jar_id: int, update: SpawnJarToTub
) -> dict[str, object]:
    try:
        grow, jar = spawn_jar_to_tub(
            settings.database_path,
            jar_id,
            tub_name=update.tub_name,
            node_id=update.node_id,
            species=update.species,
            strain=update.strain,
            spawn_to_bulk_on=update.spawn_to_bulk_on.isoformat(),
            notes=update.notes,
            spawn_ratio=update.spawn_ratio,
        )
    except KeyError as exc:
        raise HTTPException(status_code=404, detail="Spawn jar not found") from exc
    except LookupError as exc:
        raise HTTPException(status_code=404, detail="Node not found") from exc
    except (ValueError, RuntimeError) as exc:
        raise HTTPException(status_code=409, detail=str(exc)) from exc
    except sqlite3.IntegrityError as exc:
        raise HTTPException(status_code=409, detail="That tub name is already in use") from exc
    return {"grow": grow, "jar": jar}


@app.post("/api/jars/spawn", status_code=201)
def roll_spawn_jars_into_tubs(update: SpawnJarGroups) -> dict[str, object]:
    try:
        grows = spawn_jar_groups_to_tubs(
            settings.database_path,
            [
                {
                    "jar_ids": group.jar_ids,
                    "tub_name": group.tub_name,
                    "node_id": group.node_id,
                    "spawn_ratio": group.spawn_ratio,
                    "spawn_to_bulk_on": group.spawn_to_bulk_on.isoformat(),
                    "notes": group.notes,
                }
                for group in update.groups
            ],
        )
    except KeyError as exc:
        raise HTTPException(status_code=404, detail="One or more jars were not found") from exc
    except LookupError as exc:
        raise HTTPException(status_code=404, detail="Node not found") from exc
    except (ValueError, RuntimeError) as exc:
        raise HTTPException(status_code=409, detail=str(exc)) from exc
    except sqlite3.IntegrityError as exc:
        raise HTTPException(status_code=409, detail="That tub name is already in use") from exc
    return {"grows": grows}


PHOTO_MEDIA_TYPES = {
    "image/jpeg": ".jpg",
    "image/png": ".png",
    "image/webp": ".webp",
}
MAX_PHOTO_BYTES = 10 * 1024 * 1024


@app.post("/api/grows/{tub_id}/photos", status_code=201)
async def upload_grow_photo(
    tub_id: int,
    request: Request,
    filename: str = Query(min_length=1, max_length=255),
    caption: str = Query(default="", max_length=500),
    taken_on: date | None = Query(default=None),
    taken_at: datetime | None = Query(default=None),
    file_last_modified_ms: int | None = Query(default=None, ge=0),
) -> dict[str, object]:
    media_type = request.headers.get("content-type", "").split(";", 1)[0].lower()
    media_type = {"image/jpg": "image/jpeg", "image/pjpeg": "image/jpeg"}.get(
        media_type, media_type
    )
    if not media_type:
        media_type = {
            ".jpg": "image/jpeg",
            ".jpeg": "image/jpeg",
            ".png": "image/png",
            ".webp": "image/webp",
        }.get(Path(filename).suffix.casefold(), "")
    extension = PHOTO_MEDIA_TYPES.get(media_type)
    if extension is None:
        raise HTTPException(status_code=415, detail="Use a JPEG, PNG, or WebP image")
    content_length = request.headers.get("content-length")
    if content_length:
        try:
            if int(content_length) > MAX_PHOTO_BYTES:
                raise HTTPException(status_code=413, detail="Photos are limited to 10 MB")
        except ValueError as exc:
            raise HTTPException(status_code=400, detail="Invalid content length") from exc
    content = await request.body()
    if not content or len(content) > MAX_PHOTO_BYTES:
        raise HTTPException(status_code=413, detail="Photos must be between 1 byte and 10 MB")

    # Always inspect the original file. A manual date/time, when supplied,
    # intentionally overrides embedded metadata without modifying the image.
    embedded_capture = extract_capture_time(content, settings.local_timezone)
    local_zone = ZoneInfo(settings.local_timezone)
    capture: CaptureTime
    if taken_at is not None:
        aware = taken_at.replace(tzinfo=local_zone) if taken_at.tzinfo is None else taken_at
        capture = CaptureTime(
            utc=aware.astimezone(UTC),
            local_date=aware.astimezone(local_zone).date().isoformat(),
            source="manual",
        )
    elif taken_on is not None:
        aware = datetime.combine(taken_on, time(hour=12), tzinfo=local_zone)
        capture = CaptureTime(
            utc=aware.astimezone(UTC),
            local_date=taken_on.isoformat(),
            source="manual_date",
        )
    elif embedded_capture is not None:
        capture = embedded_capture
    elif file_last_modified_ms is not None:
        modified = datetime.fromtimestamp(file_last_modified_ms / 1000, tz=UTC)
        capture = CaptureTime(
            utc=modified,
            local_date=modified.astimezone(local_zone).date().isoformat(),
            source="file_modified",
        )
    else:
        uploaded = datetime.now(UTC)
        capture = CaptureTime(
            utc=uploaded,
            local_date=uploaded.astimezone(local_zone).date().isoformat(),
            source="upload_time",
        )

    stored_name = f"{uuid4().hex}{extension}"
    destination = settings.photo_directory / stored_name
    destination.write_bytes(content)
    try:
        photo = add_grow_photo(
            settings.database_path,
            tub_id,
            stored_name=stored_name,
            original_name=Path(filename).name,
            media_type=media_type,
            size_bytes=len(content),
            caption=caption,
            taken_on=capture.local_date,
            taken_at_utc=capture.utc.isoformat(),
            capture_time_source=capture.source,
        )
    except KeyError as exc:
        destination.unlink(missing_ok=True)
        raise HTTPException(status_code=404, detail="Grow not found") from exc
    return {"photo": photo}


@app.get("/api/grow-photos/{photo_id}", include_in_schema=False)
def grow_photo(photo_id: int) -> FileResponse:
    photo = get_grow_photo(settings.database_path, photo_id)
    if photo is None:
        raise HTTPException(status_code=404, detail="Photo not found")
    path = settings.photo_directory / str(photo["stored_name"])
    if not path.is_file():
        raise HTTPException(status_code=404, detail="Photo file is missing")
    return FileResponse(path, media_type=str(photo["media_type"]))


@app.post("/api/jars/{jar_id}/photos", status_code=201)
async def upload_spawn_jar_photo(
    jar_id: int,
    request: Request,
    filename: str = Query(min_length=1, max_length=255),
    caption: str = Query(default="", max_length=500),
    taken_on: date | None = Query(default=None),
    taken_at: datetime | None = Query(default=None),
    file_last_modified_ms: int | None = Query(default=None, ge=0),
) -> dict[str, object]:
    media_type = request.headers.get("content-type", "").split(";", 1)[0].lower()
    media_type = {"image/jpg": "image/jpeg", "image/pjpeg": "image/jpeg"}.get(
        media_type, media_type
    )
    if not media_type:
        media_type = {
            ".jpg": "image/jpeg", ".jpeg": "image/jpeg",
            ".png": "image/png", ".webp": "image/webp",
        }.get(Path(filename).suffix.casefold(), "")
    extension = PHOTO_MEDIA_TYPES.get(media_type)
    if extension is None:
        raise HTTPException(status_code=415, detail="Use a JPEG, PNG, or WebP image")
    content = await request.body()
    if not content or len(content) > MAX_PHOTO_BYTES:
        raise HTTPException(status_code=413, detail="Photos must be between 1 byte and 10 MB")

    embedded_capture = extract_capture_time(content, settings.local_timezone)
    local_zone = ZoneInfo(settings.local_timezone)
    capture: CaptureTime
    if taken_at is not None:
        aware = taken_at.replace(tzinfo=local_zone) if taken_at.tzinfo is None else taken_at
        capture = CaptureTime(
            utc=aware.astimezone(UTC),
            local_date=aware.astimezone(local_zone).date().isoformat(),
            source="manual",
        )
    elif taken_on is not None:
        aware = datetime.combine(taken_on, time(hour=12), tzinfo=local_zone)
        capture = CaptureTime(
            utc=aware.astimezone(UTC), local_date=taken_on.isoformat(),
            source="manual_date",
        )
    elif embedded_capture is not None:
        capture = embedded_capture
    elif file_last_modified_ms is not None:
        modified = datetime.fromtimestamp(file_last_modified_ms / 1000, tz=UTC)
        capture = CaptureTime(
            utc=modified,
            local_date=modified.astimezone(local_zone).date().isoformat(),
            source="file_modified",
        )
    else:
        uploaded = datetime.now(UTC)
        capture = CaptureTime(
            utc=uploaded,
            local_date=uploaded.astimezone(local_zone).date().isoformat(),
            source="upload_time",
        )

    stored_name = f"{uuid4().hex}{extension}"
    destination = settings.photo_directory / stored_name
    destination.write_bytes(content)
    try:
        photo = add_spawn_jar_photo(
            settings.database_path,
            jar_id,
            stored_name=stored_name,
            original_name=Path(filename).name,
            media_type=media_type,
            size_bytes=len(content),
            caption=caption,
            taken_on=capture.local_date,
            taken_at_utc=capture.utc.isoformat(),
            capture_time_source=capture.source,
        )
    except KeyError as exc:
        destination.unlink(missing_ok=True)
        raise HTTPException(status_code=404, detail="Spawn jar not found") from exc
    except PermissionError as exc:
        destination.unlink(missing_ok=True)
        raise HTTPException(status_code=423, detail=str(exc)) from exc
    return {"photo": photo}


@app.get("/api/jar-photos/{photo_id}", include_in_schema=False)
def spawn_jar_photo(photo_id: int) -> FileResponse:
    photo = get_spawn_jar_photo(settings.database_path, photo_id)
    if photo is None:
        raise HTTPException(status_code=404, detail="Photo not found")
    path = settings.photo_directory / str(photo["stored_name"])
    if not path.is_file():
        raise HTTPException(status_code=404, detail="Photo file is missing")
    return FileResponse(path, media_type=str(photo["media_type"]))


@app.delete("/api/jars/{jar_id}/photos/{photo_id}")
def remove_spawn_jar_photo(jar_id: int, photo_id: int) -> dict[str, bool]:
    try:
        photo = delete_spawn_jar_photo(settings.database_path, jar_id, photo_id)
    except PermissionError as exc:
        raise HTTPException(status_code=423, detail=str(exc)) from exc
    if photo is None:
        raise HTTPException(status_code=404, detail="Photo not found")
    (settings.photo_directory / str(photo["stored_name"])).unlink(missing_ok=True)
    return {"deleted": True}


@app.delete("/api/grows/{tub_id}/photos/{photo_id}")
def remove_grow_photo(tub_id: int, photo_id: int) -> dict[str, bool]:
    photo = delete_grow_photo(settings.database_path, tub_id, photo_id)
    if photo is None:
        raise HTTPException(status_code=404, detail="Photo not found")
    (settings.photo_directory / str(photo["stored_name"])).unlink(missing_ok=True)
    return {"deleted": True}
